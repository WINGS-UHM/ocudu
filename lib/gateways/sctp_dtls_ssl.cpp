// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
#include "sctp_dtls_ssl.h"
#include "sctp_dtls.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/error_handling.h"
#include <string>
#include <utility>

using namespace ocudu;

#ifdef OCUDU_HAVE_OPENSSL_DTLS

std::unique_ptr<dtls_ssl> ocudu::create_dtls_ssl(const dtls_ssl_config& cfg_, const dtls_ssl_dependencies& deps_)
{
  /// Creates an instance of a DTLS context.
  return std::make_unique<openssl_dtls_ssl>(cfg_, deps_);
}

openssl_dtls_ssl::openssl_dtls_ssl(const dtls_ssl_config& cfg_, const dtls_ssl_dependencies& deps_) :
  cfg(cfg_), ssl_ctx(deps_.ssl_ctx), logger(ocudulog::fetch_basic_logger("SCTP"))
{
}

openssl_dtls_ssl::~openssl_dtls_ssl()
{
  SSL_free(ssl);
}

bool openssl_dtls_ssl::init(int socket)
{
  /// Create SSL connection and BIO. We associate this BIO with the correct association at this point.
  SSL_CTX* ctx = static_cast<openssl_dtls_context&>(ssl_ctx).get_ssl_ctx();
  if (ctx == nullptr) {
    return false;
  }
  ssl = SSL_new(ctx);
  if (ssl == nullptr) {
    int err = ERR_get_error();
    logger.error("Could not initialize SSL. Cause: failure to create SSL. err={}", ERR_reason_error_string(err));
    return false;
  }
  bio = BIO_new_dgram_sctp(socket, BIO_NOCLOSE);
  if (bio == nullptr) {
    int err = ERR_get_error();
    logger.error("Could not initialize SSL. Cause: failure to create BIO. err={}", ERR_reason_error_string(err));
    return false;
  }
  SSL_set_bio(ssl, bio, bio);

  return true;
}

#else

std::unique_ptr<dtls_ssl> ocudu::create_dtls_ssl(const dtls_ssl_config& cfg_, const dtls_ssl_dependencies& deps_)
{
  report_error("Trying to create DTLS SSL association, but DTLS is not supported");
  return nullptr;
}

#endif
