# Wiring up the controller case

Written 2026-11-20. Pictures: `renders/XL_controller_wiring.png` (every wire),
`renders/XL_controller_case_detail.png` (where each board sits in the case). Pin jobs come from the app
design (the repo root). Marked **(suggested)** where a number or choice is
a suggestion I haven't settled, not from a listing or a drawing.

**Two things are not decided yet, and the steps stop at them:**

- **The Renogy jack pins.** Nothing goes onto a COMM or REMOTE pigtail wire until the jack pin numbers are
  known (not published yet). The wiring diagram draws those wires dashed.
- **Where the 12 V lead taps in.** My proposal (not approved): a fused tap at the battery side, ahead of
  the master switch, so the brain can still report with the master off. Until you pick, the lead stays
  unconnected at its far end.

No soldering anywhere: screw terminals, lever nuts and Dupont jumpers.

---

## Part A: on the bench (nothing in the box yet)

### A1. Check the parts when they arrive

1. Brain's screw base: measure its 4 holes centre to centre. Expected **64.00 x 47.97** (Lonely
   Binary's drawing). If not, stop: the case posts are drawn to those numbers.
2. DS18B20 adapter: an M3 screw should pass its 2 holes. If not, stop before mounting it.
3. Read the labels on each small board and compare them with the wiring diagram. The diagram uses
   generic names where the listings don't show them (MOSFET, PC817). If a label differs, check that
   board's own listing before wiring it.

### A2. Load the software first

The brain's USB-C port is boxed in by the MOSFET once everything is fitted, so do this before
anything is screwed down. Later updates go over Wi-Fi.

1. Get the `brain-...` and `display-...` downloads from a green **Firmware** run on GitHub (see the
   app repo's README, "Load the firmware onto the boards").
2. Brain board on its screw base, USB-C to the PC, then (your COM port in place of COM5):

```powershell
python -m esptool --chip esp32s3 --port COM5 --baud 921600 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

3. Same again with the `display-...` files on the ELECROW screen.

### A3. Fit the boards in the case base

Positions are on the detail sheet.

| Board | Where in the case | Fixing |
|---|---|---|
| ESP32-S3 brain on its screw base | front left, USB end to the right (toward the MOSFET) | 4 x M3 x 10 into the posts |
| PC817 4-channel | right, middle | 4 x M3 x 10 |
| DS18B20 adapter | back, middle, next to the probe slot | 2 x M3 x 10 |
| RS485 adapter | back left, in front of the left D hole | press into the snap brackets |
| 5 V buck | front right, USB-A socket toward the right end slot | press into the snap brackets |
| MOSFET | middle, between the brain and the PC817 | press into the snap brackets |

Don't overtighten the M3s: they cut their own thread in 2.8 mm pilots.

### A4. Low-voltage wires inside the case

Bare ends go into the brain base's screw terminals, Dupont jumpers onto the small boards' pins.
Check every pin name against the label on the brain base before tightening.

| From (brain base) | To | Wire job |
|---|---|---|
| 5V / VIN | buck **5V out +** (screw output) | 5 V in for the brain |
| GND | buck **5V out -** | ground |
| GPIO 4 | MOSFET **signal +** | fan on/off and speed |
| GND | MOSFET **signal -** | |
| 3V3 | RS485 **VCC** | |
| GPIO 17 (TX) | RS485 **TXD** | serial out |
| GPIO 18 (RX) | RS485 **RXD** | serial in |
| GND | RS485 **GND** | |
| 3V3 | DS18B20 adapter **VCC** | |
| GPIO 5 | DS18B20 adapter **DAT** | temperature |
| GND | DS18B20 adapter **GND** | |
| GPIO 6 | PC817 **IN1** | inverter on/off "press" |
| GPIO 7 | PC817 **OUT2** | inverter ON light |
| GPIO 15 | PC817 **OUT3** | inverter ERR light |
| 3V3 | PC817 **V** (pull-up) | **(suggested)** |
| GND | PC817 **G** (input side) | |

GPIO 16 stays free for an RS485 direction pin if the adapter ever needs one **(suggested)**. PC817
channel 4 is spare.

The RS485 adapter's TXD/RXD labels can mean either side. If the Renogy units never answer later, swap
the GPIO 17 and GPIO 18 wires. Swapping them does no harm.

### A5. Bench power test

Power from a 12 V supply, or the fused lead (A5.1) clipped to the battery.

1. Make up the 12 V lead: the inline fuse holder (18 AWG) with a **3 A** blade fuse **(suggested)**,
   red to + and black to -. Leave the fuse **out** until step 3.
2. **Take the brain's 5V / VIN wire off** the buck for now.
3. 12 V into buck **IN + / IN -**, fuse in. Meter on DC volts across buck **5V out + / -**: it should
   read about 5.0 V. **Stop if it reads over 5.3 V**, and don't connect the brain.
4. Fuse out, brain's 5V / VIN wire back on, fuse in. The brain's LED should light. Plug the USB-A to C
   lead from the buck's USB-A socket into the screen: it should start.
5. PC817 channel 1 at 3.3 V: meter on beep across PC817 **OUT1** and **G** (output side). Move the
   IN1 jumper from GPIO 6 to a 3V3 pin. It should beep, and stop when the jumper comes off. The PC817
   listing gives 3.6 to 24 V in, so 3.3 V is a little under it. If it doesn't beep, stop before
   going on. Put the jumper back on GPIO 6.
6. Fuse out.

---

## Part B: in the box

**Before you start:** master switch **off**, and the 3 A fuse **out** of the 12 V lead.

The inverter's COMM and REMOTE jacks are on its left end, under the top plate: they can only be reached
with the plate lifted (install order, step 4). Plug in the pigtails while the plate is up.

### B1. Make room

1. Take the two BT-2s off the plate and unplug them from their jacks. Keep them as backups, with the
   round BT sensor in front of the left one.
2. Move the loose wire at the right-front corner of the case place. It sits where the buck goes.

### B2. Wires up through the plate

1. DC-DC charger **COMM** jack: one RJ45 pigtail, wires up the **right D hole**.
2. Inverter **COMM** jack: the other RJ45 pigtail, wires up the **left D hole**.
3. Inverter **REMOTE** jack: the RJ11 pigtail, up the **left D hole**. The wired remote comes out of
   that jack and is kept whole as a backup. The inverter's own switch stays on **REM**.
4. DS18B20 probe: from the bottom up through **hole Q**, then into the case through the back-wall
   probe slot.
5. Fan leads: extend both fans' leads from the back wall forward to the case (lever nuts on the joins).

Label every wire end as it comes up. The D holes open inside the case floor, so the pigtails go
straight up into the case.

### B3. Case on the plate

1. Feed the pigtails and the probe through as the case goes down over the D holes.
2. 4 x M5 countersunk (about 8 long) through the case floor into the 4 old BT-2 holes. The heads
   sit flush in the 4 mm floor.

### B4. 12 V and the fans (right-end 12 V/fan slot)

1. Fused 12 V lead in through the 12 V/fan slot. Fuse still **out**.
2. Two lever nuts in the lever-nut place (front middle of the case): one **12 V +** and one **GND**.
3. Onto the **12 V +** lever nut: lead red, buck **IN +**, MOSFET **load 12V+**, both fans' **red**.
4. Onto the **GND** lever nut: lead black, buck **IN -**, MOSFET **load GND**.
5. Both fans' **black** to MOSFET **load OUT-**.
6. The lead's far end goes to the tap point **once you have picked it**.

### B5. Jack wires (only once the pin numbers are known)

1. Wait for the updated wiring diagram with the pin numbers.
2. RS485 screw block: **A+** to the A wire of **both** COMM pigtails, **B-** to both B wires, **G** to
   both GND wires.
3. REMOTE pigtail: the switch pair to PC817 **OUT1 / G** (output side); the ON and ERR light pairs to
   PC817 **IN2 / IN3**. Which wires these are comes from your step 4 readings.
4. **Every other pigtail wire** goes into its own terminal on the cap strip (back right of the case).
   Nothing is joined to anything else. The **~5 V pin (pin 8 on the COMM jacks)** never goes to the
   brain.

### B6. Screen

USB-A to C lead from the buck's USB-A socket, out the right-end USB slot, to the screen on its outside
wall. Which wall it goes on is not chosen yet.

### B7. Switch on and check

1. Fuse in. The brain's LED should light and the screen should start.
2. The screen and the app should show the box temperature. Once B5 is done, they should also show the
   DC-DC and inverter numbers.
3. On the screen, set the fans to manual and move the speed slider up: both fans should start. Set them
   back to auto (speed follows the box temperature).
4. Lid on: it snaps on with 6 tongues; the vent sits over the buck.

---

## If something is wrong

- **No 5 V at the buck:** check the fuse and the 12 V + / GND lever nuts.
- **The brain resets when the fans start:** stop and find the cause. Don't change the fuse size.
- **The DC-DC or inverter never answers:** swap the GPIO 17 / 18 wires (A4). Then check A+ / B- against
  the pin numbers. A and B swapped does no harm either.
- **Anything warm that shouldn't be:** fuse out, and find the cause before going on.
