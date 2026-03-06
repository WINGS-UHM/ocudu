#!/usr/bin/env python3

from contextlib import suppress
import os
import json
from time import sleep
import websocket
import matplotlib.pyplot as plt
import numpy as np
os.environ["WS_URL"] = "localhost:8001"
plt.ion()

fig, axs = plt.subplots(3, 1, figsize=(6, 8))

const_scatter = axs[0].scatter([], [])
axs[0].set_title("Constellation")
axs[0].set_xlabel("I")
axs[0].set_ylabel("Q")
axs[0].grid(True)

csi_mag_line, = axs[1].plot([], [])
axs[1].set_title("CSI Magnitude")
axs[1].grid(True)

csi_phase_line, = axs[2].plot([], [])
axs[2].set_title("CSI Phase")
axs[2].grid(True)

plt.tight_layout()


def update_plot(metric):

    const_i = np.array(metric["constellation"]["i"])
    const_q = np.array(metric["constellation"]["q"])

    csi_mag = np.array(metric["csi"]["mag"])
    csi_phase = np.array(metric["csi"]["phase"])

    axs[0].cla()
    axs[0].scatter(const_i, const_q)
    axs[0].set_title("Constellation")
    axs[0].set_xlabel("I")
    axs[0].set_ylabel("Q")
    axs[0].grid(True)

    axs[1].cla()
    axs[1].plot(csi_mag)
    axs[1].set_title("CSI Magnitude")
    axs[1].grid(True)

    axs[2].cla()
    axs[2].plot(csi_phase)
    axs[2].set_title("CSI Phase")
    axs[2].grid(True)

    plt.pause(0.001)


def _on_open(ws: websocket.WebSocketApp):
    ws.send(json.dumps({"cmd": "metrics_subscribe"}))


def _on_message(_ws: websocket.WebSocketApp, message: str):

    with suppress(json.JSONDecodeError):

        metric = json.loads(message)

        if metric.get("type") == "isac":
            update_plot(metric)

        else:
            print(json.dumps(metric))


if __name__ == "__main__":

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
        ws_app.close()