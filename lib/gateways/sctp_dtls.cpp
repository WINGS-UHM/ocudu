// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
#include "sctp_dtls.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/error_handling.h"
#include <string>
#include <utility>

using namespace ocudu;

#ifdef OCUDU_HAVE_OPENSSL_DTLS

static int verify_callback(int ok, X509_STORE_CTX* store)
{
  // TODO for now, always trust.
  return 1;
}

/// Creates an instance of a DTLS context.
std::unique_ptr<dtls_context> ocudu::create_dtls_context(dtls_context_config cfg_)
{
  return std::make_unique<openssl_dtls_context>(cfg_);
}

/// DTLS context instance
openssl_dtls_context::openssl_dtls_context(dtls_context_config cfg_) :
  cfg(std::move(cfg_)), logger(ocudulog::fetch_basic_logger("SCTP"))
{
}

openssl_dtls_context::~openssl_dtls_context()
{
  if (ssl_ctx != nullptr) {
    SSL_CTX_free(ssl_ctx);
  }
}

bool openssl_dtls_context::init(int socket)
{
  // Create SSL context. We will decide later if a particular SSL
  // session will act as server or client.
  ssl_ctx = SSL_CTX_new(DTLS_method());
  if (ssl_ctx == nullptr) {
    unsigned long err = ERR_get_error();
    logger.error("Could not initialize DTLS context. Cause: failure to create context. session={} err={}",
                 cfg.session_id,
                 ERR_reason_error_string(err));
    return false;
  }

  // Set session ID of DTLS context.
  if (!SSL_CTX_set_session_id_context(ssl_ctx, (const unsigned char*)cfg.session_id.c_str(), cfg.session_id.size())) {
    unsigned long err = ERR_get_error();
    logger.error("Could not initialize DTLS context. Cause: failure to set session id. session={} err={}",
                 cfg.session_id,
                 ERR_reason_error_string(err));
    return false;
  }

  // Load certificate from file.
  if (!SSL_CTX_use_certificate_file(ssl_ctx, cfg.cert_filename.c_str(), SSL_FILETYPE_PEM)) {
    unsigned long err = ERR_get_error();
    logger.error("Could not initialize DTLS context. Cause: invalid certificate file. session={} filename={} err={}",
                 cfg.session_id,
                 cfg.cert_filename,
                 ERR_reason_error_string(err));
    return false;
  }

  // Load private key from file.
  if (!SSL_CTX_use_PrivateKey_file(ssl_ctx, cfg.key_filename.c_str(), SSL_FILETYPE_PEM)) {
    unsigned long err = ERR_get_error();
    logger.error("Could not initialize DTLS context. Cause: invalid key file. session={} filename={} err={}",
                 cfg.session_id,
                 cfg.key_filename,
                 ERR_reason_error_string(err));
    return false;
  }

  // Check private key is valid.
  if (!SSL_CTX_check_private_key(ssl_ctx)) {
    unsigned long err = ERR_get_error();
    logger.error("Could not initialize DTLS context. Cause: invalid key. session={} filename={} err={}",
                 cfg.session_id,
                 cfg.key_filename,
                 ERR_reason_error_string(err));
    return false;
  }

  // Set verify callback.
  SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_callback);

  // Create BIO to set all necessary parameters for following connections, e.g. SCTP-AUTH.
  // Will not be used.
  BIO* bio = BIO_new_dgram_sctp(socket, BIO_NOCLOSE);
  if (!bio) {
    unsigned long err = ERR_get_error();
    logger.error("Could not initialize DTLS context. Cause: failed to configure socket. session={} filename={} err={}",
                 cfg.session_id,
                 cfg.key_filename,
                 ERR_reason_error_string(err));
    return false;
  }
  BIO_free(bio);
  return true;
}

#else

std::unique_ptr<dtls_context> ocudu::create_dtls_context(dtls_context_config cfg_)
{
  report_error("Trying to create DTLS context, but DTLS is not supported");
  return nullptr;
}

#endif
