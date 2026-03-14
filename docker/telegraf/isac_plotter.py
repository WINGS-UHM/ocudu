#!/usr/bin/env python3

from contextlib import suppress
from collections import deque
import os
import json
from time import sleep

import matplotlib.pyplot as plt
import numpy as np
import websocket
import pickle

# Check for WS_URL
if os.environ.get("WS_URL") is None:
    os.environ["WS_URL"] = "localhost:8001"  # Default WebSocket URL 
plt.ion()

# Rolling-window configuration
CONST_WINDOW = 2000
CSI_TIME_WINDOW = 500
CSI_INDEX = 10
CSI_GRID = []
# If no new data arrives for this many websocket reconnect failures, stop trying.
MAX_RECONNECTS = 1
RECONNECT_DELAY_SEC = 1.0
CSI_BIN_PATH = "csi_grid.pkl"

# Rolling buffers
const_i_buf = deque(maxlen=CONST_WINDOW)
const_q_buf = deque(maxlen=CONST_WINDOW)

time_buf = deque(maxlen=CSI_TIME_WINDOW)
csi_mag_buf = deque(maxlen=CSI_TIME_WINDOW)
csi_phase_buf = deque(maxlen=CSI_TIME_WINDOW)

sample_counter = 0
stop_requested = False


fig, axs = plt.subplots(3, 1, figsize=(7, 9))

const_scatter = axs[0].scatter([], [], s=12)
axs[0].set_title("Constellation (rolling window)")
axs[0].set_xlabel("I")
axs[0].set_ylabel("Q")
axs[0].grid(True)
axs[0].set_aspect("equal", adjustable="box")
axs[0].set_xlim(-2, 2)
axs[0].set_ylim(-2, 2)

csi_mag_line, = axs[1].plot([], [])
axs[1].set_title(f"CSI Magnitude vs Time (RE index {CSI_INDEX})")
axs[1].set_xlabel("Snapshot")
axs[1].set_ylabel("|H|")
axs[1].grid(True)

csi_phase_line, = axs[2].plot([], [])
axs[2].set_title(f"CSI Phase vs Time (RE index {CSI_INDEX})")
axs[2].set_xlabel("Snapshot")
axs[2].set_ylabel("∠H (rad, unwrapped)")
axs[2].grid(True)

plt.tight_layout()


def _safe_set_ylim(ax, y):
    """Set y-limits safely for empty or constant data."""
    if len(y) == 0:
        return

    ymin = float(np.min(y))
    ymax = float(np.max(y))

    if not np.isfinite(ymin) or not np.isfinite(ymax):
        return

    if ymin == ymax:
        ymin -= 0.1
        ymax += 0.1

    pad = 0.05 * (ymax - ymin)
    ax.set_ylim(ymin - pad, ymax + pad)


def redraw_from_buffers():
    """Draws the plots from the current contents of the rolling buffers."""
    # --- Constellation ---
    if len(const_i_buf) > 0 and len(const_q_buf) > 0:
        n_const = min(len(const_i_buf), len(const_q_buf))
        pts = np.column_stack([
            np.array(list(const_i_buf)[:n_const], dtype=float),
            np.array(list(const_q_buf)[:n_const], dtype=float),
        ])
        const_scatter.set_offsets(pts)

        lim = max(
            np.max(np.abs(pts[:, 0])) if pts.shape[0] > 0 else 1.5,
            np.max(np.abs(pts[:, 1])) if pts.shape[0] > 0 else 1.5,
            1.5,
        ) * 1.1
        axs[0].set_xlim(-lim, lim)
        axs[0].set_ylim(-lim, lim)
        axs[0].set_title(f"Constellation (last {n_const} pts)")
    else:
        const_scatter.set_offsets(np.empty((0, 2)))
        axs[0].set_title("Constellation (no data)")

    # --- CSI magnitude over time ---
    if len(time_buf) > 0 and len(csi_mag_buf) > 0:
        n_mag = min(len(time_buf), len(csi_mag_buf))
        x_mag = np.array(list(time_buf)[:n_mag], dtype=float)
        y_mag = np.array(list(csi_mag_buf)[:n_mag], dtype=float)

        csi_mag_line.set_data(x_mag, y_mag)
        if len(x_mag) == 1:
            axs[1].set_xlim(x_mag[0], x_mag[0] + 1)
        else:
            axs[1].set_xlim(x_mag[0], x_mag[-1])

        _safe_set_ylim(axs[1], y_mag)
        axs[1].set_title(f"CSI Magnitude vs Time (RE index {CSI_INDEX})")
    else:
        csi_mag_line.set_data([], [])
        axs[1].set_title("CSI Magnitude vs Time (no data)")

    # --- CSI phase over time ---
    if len(time_buf) > 0 and len(csi_phase_buf) > 0:
        n_phase = min(len(time_buf), len(csi_phase_buf))
        x_phase = np.array(list(time_buf)[:n_phase], dtype=float)
        y_phase = np.unwrap(np.array(list(csi_phase_buf)[:n_phase], dtype=float))

        csi_phase_line.set_data(x_phase, y_phase)
        if len(x_phase) == 1:
            axs[2].set_xlim(x_phase[0], x_phase[0] + 1)
        else:
            axs[2].set_xlim(x_phase[0], x_phase[-1])

        _safe_set_ylim(axs[2], y_phase)
        axs[2].set_title(f"CSI Phase vs Time (RE index {CSI_INDEX})")
    else:
        csi_phase_line.set_data([], [])
        axs[2].set_title("CSI Phase vs Time (no data)")

    fig.canvas.draw_idle()
    plt.pause(0.001)


def update_plot(metric):
    global sample_counter
    
    # Constellation from current snapshot
    const_i = np.asarray(metric.get("constellation", {}).get("i", []), dtype=float)
    const_q = np.asarray(metric.get("constellation", {}).get("q", []), dtype=float)

    # Guard against mismatched lengths
    n_const = min(len(const_i), len(const_q))
    if n_const > 0:
        for x, y in zip(const_i[:n_const], const_q[:n_const]):
            const_i_buf.append(float(x))
            const_q_buf.append(float(y))

    # Arrange into Real+Imag CSI array
    csi_real = np.asarray(metric.get("csi", {}).get("mag", []), dtype=float)
    csi_imag = np.asarray(metric.get("csi", {}).get("phase", []), dtype=float)

    n_csi = min(len(csi_real), len(csi_imag))
    if n_csi > 0:
        H = csi_real[:n_csi] + 1j * csi_imag[:n_csi]
        CSI_GRID.append(H.tolist())

        # Guard CSI index
        idx = min(max(CSI_INDEX, 0), n_csi - 1)

        h_sel = H[idx]
        sample_counter += 1
        time_buf.append(sample_counter)
        csi_mag_buf.append(float(np.abs(h_sel)))
        csi_phase_buf.append(float(np.angle(h_sel)))

    redraw_from_buffers()


def _on_open(ws: websocket.WebSocketApp):
    print("\033[1;32m[INFO] WebSocket connected\033[0m")
    ws.send(json.dumps({"cmd": "metrics_subscribe"}))


def _on_message(_ws: websocket.WebSocketApp, message: str):
    with suppress(json.JSONDecodeError, TypeError, ValueError, KeyError):
        metric = json.loads(message)
        if metric.get("type") == "isac":
            update_plot(metric)


def _on_error(_ws: websocket.WebSocketApp, error):
    print(f"\033[1;31m[ERROR] WebSocket error: {error}\033[0m")


def _on_close(_ws: websocket.WebSocketApp, close_status_code, close_msg):
    global stop_requested
    print(f"\033[1;33m[WARNING] WebSocket closed: code={close_status_code}, msg={close_msg}\033[0m")
    stop_requested = True


if __name__ == "__main__":
    ws_url = "ws://" + os.environ["WS_URL"]
    reconnects = 0

    try:
        while True:
            stop_requested = False

            ws_app = websocket.WebSocketApp(
                ws_url,
                on_open=_on_open,
                on_message=_on_message,
                on_error=_on_error,
                on_close=_on_close,
            )

            ws_app.run_forever()

            # Draw any remaining buffered data before attempting to reconnect or exit.
            redraw_from_buffers()

            reconnects += 1
            if reconnects > MAX_RECONNECTS:
                print("\033[1;32m[INFO] No more reconnect attempts. Holding final buffered plot.\033[0m")
                break

            print(f"\033[1;32m[INFO] Reconnecting in {RECONNECT_DELAY_SEC} s\033[0m")
            sleep(RECONNECT_DELAY_SEC)

    except KeyboardInterrupt:
        print("\033[1;33m[WARNING] Interrupted by user.\033[0m")

    finally:
        # Final draw and keep the last buffered points visible.
        redraw_from_buffers()
        plt.ioff()
        max_re = 0
        min_re = 1000000000
        for i in range(len(CSI_GRID)):
            if len(CSI_GRID[i]) > max_re:
                max_re = len(CSI_GRID[i])
            if len(CSI_GRID[i]) < min_re:
                min_re = len(CSI_GRID[i])
        print(f"\033[1;32m[INFO] Total CSI snapshots: {len(CSI_GRID)}\033[0m")
        with open(CSI_BIN_PATH, "wb") as f:
            pickle.dump(CSI_GRID, f)
        print(f"\033[1;32m[INFO] Saving CSI data to {CSI_BIN_PATH}, Max RE={max_re}, Min RE={min_re}\033[0m")
        print("\033[1;32m[INFO] Plotting Remaining Data, Close window to exit.\033[0m")
        plt.show()