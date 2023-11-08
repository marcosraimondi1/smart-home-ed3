import serial, time
import datetime as dt
import threading
from tkinter import *

# COLORS -------------------------------------
blue = "#4D9DE0"
red = "#E15554"
yellow = "#E1BC29"
grey = "#d1d1cf"
white = "#f9f6f2"

font = "Console"

# VENTANA PRINCIPAL ------------------------------------------------------------

root = Tk()  # create root window
root.title("PICSON")  # title of the GUI window
root.maxsize(800, 600)  # specify the max size the window can expand to
root.minsize(800, 600)  # specify the max size the window can expand to
root.config(bg=white)  # specify background color

# -------------------------------------------------------------------------------

# VARIABLES ---------------------------------------------------------------------

# -------------------------------------------------------------------------------

pic = serial.Serial()
port = StringVar(root)
connectionStatus = StringVar(root, "Disconnected")
connectionButton = StringVar(root, "connect")
last_msg = StringVar(root)
sendFreq = IntVar(root, 0)
sendFreqId = 42
upLimit = IntVar(root, 0)
upLimitId = 21
downLimit = IntVar(root, 0)
downLimitId = 15
messages = []

# -------------------------------------------------------------------------------

# FUNCIONES ---------------------------------------------------------------------

# -------------------------------------------------------------------------------


def intATemperatura(x):
    # la tension es Vout = X * Vref / 2^N
    # donde X es la salida del ADC
    # N es la cantidad de bits del ADC
    # Vref es la tension de referencia
    Vout = x * 5 / 1024
    Temp = Vout * 100

    return Temp


def addMessage(msg):
    # agrega mensaje a la lista de mensajes a mostrar
    global messages
    messages.insert(
        0,
        msg + "\t-\t" + str(dt.datetime.now()).split(".")[0].split(" ")[1],
    )
    if len(messages) > 5:
        messages = messages[:5]

    msgs_string = ""

    for m in messages:
        msgs_string += "\n>  " + m

    last_msg.set(msgs_string)


def listenPort():
    # escucha el puerto serie y agrega los mensajes recibidos
    messages.clear()
    addMessage("listening on port " + port.get())
    try:
        while True:
            if not pic.is_open:
                return
            # bytesToRead = pic.inWaiting()
            msg = pic.read(1)
            if msg:
                valor = int.from_bytes(msg, "little")
                temp = intATemperatura(valor * 4)
                addMessage(str(valor) + " = " + str(round(temp, 2)) + " °C")

    except Exception as e:
        print(e)
        messages.clear()
        addMessage("port disconnected")


def handleConnection():
    # conecta o desconecta dispositivo del puerto serie
    messages.clear()
    if pic.is_open:
        addMessage("disconnected")
        pic.close()
        connectionStatus.set("Disconnected")
        connectionButton.set("connect")
        return

    try:
        pic.setPort(port.get())
        pic.timeout = 1
        pic.open()
        time.sleep(2)
        connectionStatus.set("Connected")
        connectionButton.set("disconnect")

        # initialize serial port listener
        listenThread = threading.Thread(target=listenPort)
        listenThread.daemon = True
        listenThread.start()

    except serial.SerialException:
        connectionStatus.set("Failed to Connect: check port connection")

    return


def limitVar(x):
    # limita x entre 1 y 255
    x = x if x <= 255 else 255
    x = x if x >= 1 else 1
    return x


def handleLoadVariable(x, varId):
    # cargar variables por puerto serial
    x = limitVar(x)
    if pic.is_open:
        pic.write(varId.to_bytes(byteorder="little"))
        pic.write(x.to_bytes(byteorder="little"))

    return


# -------------------------------------------------------------------------------

# ESTRUCTURA --------------------------------------------------------------------

# -------------------------------------------------------------------------------

# title
Label(root, text="PICSON CONFIG PANEL", font=(font, 14), bg=white).grid(
    row=0, column=0, padx=5, pady=5
)

# connection panel
Label(root, text="Port:", font=(font, 12), bg=white).grid(
    row=1, column=0, padx=5, pady=5
)

Entry(root, textvariable=port, font=(font, 12), bg=white).grid(
    row=1, column=1, padx=5, pady=5
)

Button(
    root,
    textvariable=connectionButton,
    font=(font, 12),
    command=handleConnection,
    bg=grey,
).grid(row=1, column=2, padx=5, pady=5)

# connection Status
Label(root, text="Status:", font=(font, 12), bg=white).grid(
    row=2, column=0, padx=5, pady=5
)

Label(root, textvariable=connectionStatus, font=(font, 10), bg=white).grid(
    row=2, column=1, padx=5, pady=5
)

# send freq
Label(root, text="Send Freq:", font=(font, 12), bg=white).grid(
    row=3, column=0, padx=5, pady=5
)
Entry(root, textvariable=sendFreq, font=(font, 12), bg=white).grid(
    row=3, column=1, padx=5, pady=5
)

Button(
    root,
    text="load",
    font=(font, 12),
    command=lambda: handleLoadVariable(sendFreq.get(), sendFreqId),
    bg=grey,
).grid(row=3, column=2, padx=5, pady=5)

Label(
    root,
    text="Se envía una notificacion cada X seg (1-255 seg)",
    font=(font, 10),
    bg=white,
).grid(row=4, column=1, padx=5, pady=5)


# limite superior
Label(root, text="Limite Superior:", font=(font, 12), bg=white).grid(
    row=5, column=0, padx=5, pady=5
)
Entry(root, textvariable=upLimit, font=(font, 12), bg=white).grid(
    row=5, column=1, padx=5, pady=5
)

Button(
    root,
    text="load",
    font=(font, 12),
    command=lambda: handleLoadVariable(upLimit.get(), upLimitId),
    bg=grey,
).grid(row=5, column=2, padx=5, pady=5)

Label(
    root,
    text="Si variable > limite, se envía una notificacion (1-255)",
    font=(font, 10),
    bg=white,
).grid(row=6, column=1, padx=5, pady=5)


# limite inferior
Label(root, text="Limite Inferior:", font=(font, 12), bg=white).grid(
    row=7, column=0, padx=5, pady=5
)
Entry(root, textvariable=downLimit, font=(font, 12), bg=white).grid(
    row=7, column=1, padx=5, pady=5
)

Button(
    root,
    text="load",
    font=(font, 12),
    command=lambda: handleLoadVariable(downLimit.get(), downLimitId),
    bg=grey,
).grid(row=7, column=2, padx=5, pady=5)

Label(
    root,
    text="Si variable < limite, se envía una notificacion (1-255)",
    font=(font, 10),
    bg=white,
).grid(row=8, column=1, padx=5, pady=5)


# serial monitor
Label(root, text="Serial Monitor:", font=(font, 12), bg=white).grid(
    row=12, column=0, padx=5, pady=5
)
Label(root, textvariable=last_msg, font=(font, 10), bg=white).grid(
    row=13, column=1, padx=5, pady=5
)

root.mainloop()
