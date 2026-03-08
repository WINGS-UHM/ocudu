#!/usr/bin/env python3

from contextlib import suppress
import os
import json
from time import sleep

import matplotlib.pyplot as plt
import numpy as np
import websocket

os.environ["WS_URL"] = "localhost:8001"

plt.ion()

fig, axs = plt.subplots(3, 1, figsize=(7, 9))

# Constellation panel
axs[0].set_title("Constellation")
axs[0].set_xlabel("I")
axs[0].set_ylabel("Q")
axs[0].grid(True)
axs[0].set_aspect("equal", adjustable="box")
const_scatter = axs[0].scatter([], [])

# CSI magnitude panel
axs[1].set_title("CSI Magnitude")
axs[1].set_xlabel("Subcarrier / RE index")
axs[1].set_ylabel("|H|")
axs[1].grid(True)
csi_mag_line, = axs[1].plot([], [])

# CSI phase panel
axs[2].set_title("CSI Phase")
axs[2].set_xlabel("Subcarrier / RE index")
axs[2].set_ylabel("∠H (rad)")
axs[2].grid(True)
csi_phase_line, = axs[2].plot([], [])

plt.tight_layout()


def update_plot(metric):
    const_i = np.asarray(metric.get("constellation", {}).get("i", []), dtype=float)
    const_q = np.asarray(metric.get("constellation", {}).get("q", []), dtype=float)

    # These are currently raw CSI real/imag, despite the field names.
    csi_real = np.asarray(metric.get("csi", {}).get("mag", []), dtype=float)
    csi_imag = np.asarray(metric.get("csi", {}).get("phase", []), dtype=float)

    # Reconstruct complex CSI.
    n_csi = min(len(csi_real), len(csi_imag))
    if n_csi > 0:
        H = csi_real[:n_csi] + 1j * csi_imag[:n_csi]
        csi_mag = np.abs(H)
        csi_phase = np.unwrap(np.angle(H))
    else:
        csi_mag = np.array([], dtype=float)
        csi_phase = np.array([], dtype=float)

    # --- Constellation ---
    axs[0].cla()
    axs[0].scatter(const_i, const_q, s=20)
    axs[0].set_title(f"Constellation ({len(const_i)} pts)")
    axs[0].set_xlabel("I")
    axs[0].set_ylabel("Q")
    axs[0].grid(True)
    axs[0].set_aspect("equal", adjustable="box")

    if len(const_i) > 0 and len(const_q) > 0:
        lim = max(np.max(np.abs(const_i)), np.max(np.abs(const_q))) * 1.2
        if lim > 0:
            axs[0].set_xlim(-lim, lim)
            axs[0].set_ylim(-lim, lim)

    # --- CSI Magnitude ---
    axs[1].cla()
    axs[1].plot(csi_mag)
    axs[1].set_title(f"CSI Magnitude ({len(csi_mag)} pts)")
    axs[1].set_xlabel("Subcarrier / RE index")
    axs[1].set_ylabel("|H|")
    axs[1].grid(True)

    # --- CSI Phase ---
    axs[2].cla()
    axs[2].plot(csi_phase)
    axs[2].set_title(f"CSI Phase ({len(csi_phase)} pts)")
    axs[2].set_xlabel("Subcarrier / RE index")
    axs[2].set_ylabel("∠H (rad, unwrapped)")
    axs[2].grid(True)

    fig.canvas.draw_idle()
    plt.pause(0.001)


def _on_open(ws: websocket.WebSocketApp):
    ws.send(json.dumps({"cmd": "metrics_subscribe"}))


def _on_message(_ws: websocket.WebSocketApp, message: str):
    with suppress(json.JSONDecodeError):
        metric = json.loads(message)

        if metric.get("type") == "isac":
            update_plot(metric)
            print(json.dumps(metric))
        else:
            pass


if __name__ == "__main__":
    ws_app = None
    try:
        ws_app = websocket.WebSocketApp(
            "ws://" + os.environ["WS_URL"],
            on_open=_on_open,
            on_message=_on_message,
        )

        while ws_app.run_forever():
            sleep(1)

    except Exception as e:
        print(f"Error: {e}\nExiting...")

    except KeyboardInterrupt:
        print("Exiting...")

    finally:
        if ws_app is not None:
            ws_app.close()