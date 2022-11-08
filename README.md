# Windows Volume Mixer

A physical hardware interface for adjusting main and per-app volumes in windows. 

## Hardware

All hardware is presented in a desktop unit mounted at an angle for ergonomic use. The mixer consists of four channels. Each channel has a small screen for displaying the name of the channel, a rotary encoder knob for controlling the volume of that channel and ring of RGB LEDs around the knob to display the volume level of the channel. Below the knobs is a bank of key switches that can control aspects of the mixer as well other functions in windows. The mixer is connected to the windows PC via a USB-C cable for power and communications.

I2C is used for the screens, encoders, and buttons. All devices are connected to the microcontroller via cables and connectors, resulting minimal soldering needed for the construction. Once the initial soldering is completed, the whole unit can be assembled and disassembled with no soldering needed.
The enclosure for the electronics is 3D-printable on consumer printer and uses minimal M2/M3 hardware for assembly.

## Software

The software for the mixer consists of two main parts: firmware running on the microcontroller and a python script running on the PC. The two programs communicate via serial over a USB cable. The firmware reads the values of the rotary encoders and the various buttons and reports them when they change to the PC over serial. The python scripts then process these values, set appropriate volumes on the PC, decides what apps will be assigned to what channels etc. The script then sends values to be displayed on the screens and the LED rings back to the microcontroller.

The python script uses the Pycaw library to access the windows APIs to get and change app specific volumes.

# Current State of the Project and Development Plans

The project is a working, but early state. The software especially has some instability and many features to be implemented.

## Hardware

The electronics are mostly final state. Since the components were chosen to facilitate fast prototyping, the overall cost of them was fairly high. This could be improved by incorporating more custom built parts or moving most components to a single custom PCB. However, this would be impractical outside larger scale production so therefore unlikely in the near future for this project.

The current 3D-printed enclosure consists of a solid faceplate mounted on an open frame. A fully enclosed design could add more polish to the project.

##Software

Software/firmware is where most of the remaining work is to be done on the project. The firmware on the microcontroller is fairly complete and has no know bugs at this time. The python script running on the pc has a few know issues and quite a few features to be implemented.

Know issues:
•	Intermittent crashes when switching audio outputs in windows.
•	Intermittent loss of master volume control.
•	Random script crashes.

Missing features:
•	When volumes or mutes are changed in windows, update mixer automatically.
•	When output source is changed in windows, update mixer automatically.
•	Display more info about channels on screens.
•	Add more functions to buttons, including changing audio output sources.
•	Add more user feedback using button backlights.
•	Run python script in background(in task bar).
•	Add GUI to view connection status and adjust settings





