# AssaultCube C++ Mod Menu

A lightweight, internal mod menu for **AssaultCube** written in C++. This project demonstrates basic game memory manipulation, process attachment, and pointer scanning techniques for educational purposes.

---

## 🚀 Features

*   **Infinite Health:** Lock your health value to prevent taking damage.
*   **Infinite Ammo:** Keep your primary and secondary weapon ammunition full.
*   **Infinite Grenades:** Throw as many grenades as you want without running out.
*   **Clean window.h Overlay:** An intuitive in-game menu interface that can be toggled on and off.

---

## 🛠️ Prerequisites & Requirements

To run this mod menu, you will need:

*   **Operating System:** Windows 10/11
*   **Target Game:** [AssaultCube](https://assault.cubers.net/) (v1.2 or compatible).
*   **Main .exe:** Menu files [Main .exe file]
---

## 📂 Project Structure

```text
├── /
│   ├── main.cpp        # Main file
│   ├── main.exe        # Main exe (Is this the exe for Mod Menu)
│   ├── infhealth.cpp   # Gives inf Health Run Command: infhealth.exe <AMOUNT>
│   ├── infamo.cpp      # Gives inf Ammo Run Command: infamo.exe <AMOUNT>
│   └── infgrenade.cpp # Gives one grenade then freezes the value (Inf) Run Command: infgrenade.exe <1 is for enable, 0 is for disable>
├── build/
│   ├── infhealth.exe # Plug and play
│   ├── infamo.exe    # Plug and play
│   └── infgrenade.exe # Plug and play
└── README.md
```
