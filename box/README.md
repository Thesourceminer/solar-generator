# Solar Generator box: parts and instructions

A 12 V power box built into a Milwaukee PACKOUT XL Tool Box (48-22-8429): a Renogy 100 Ah LiFePO4 battery, a 1000 W inverter, a DC-DC charger and an Auxbeam switch panel, held in printed parts.

| File | What |
|---|---|
| [XL_Solar_Generator_Build.pdf](XL_Solar_Generator_Build.pdf) | **Start here.** The build book: what goes in the box, every part to print, hardware, top plate layout, wiring, the controller case, install order and the parts list |
| [stl/](stl/) | The parts to print (STL) |
| [CONTROLLER_WIRING.md](CONTROLLER_WIRING.md) | Wiring up the controller case, step by step (also in the build book) |
| [PARTS_LIST.md](PARTS_LIST.md) | What to buy, with prices and Amazon ASINs (also in the build book) |

## What to print

Printed in PETG on a Bambu Lab H2S (bed 340 x 320). Put the long side of the base and plate pieces along the 340 direction. Section 2 of the build book has the sizes and notes for each part.

| Part | STL | Prints |
|---|---|---|
| Base, left half | `XL_base_left.stl` | flat, no supports |
| Base, right half | `XL_base_right.stl` | flat, no supports |
| Seam strip (under the join of the base halves) | `XL_seam_strip.stl` | flat |
| Feet, set of 6 | `XL_feet.stl` | flat, all on one plate |
| Post 4 (back left) | `XL_post4.stl` | lying down |
| Right spacer | `XL_spacer_right.stl` | flat |
| Top plate, left piece | `XL_deck_left.stl` | flat, **supports off** (the screw holes open on the bed side) |
| Top plate, right piece | `XL_deck_right.stl` | flat, **supports off** |
| Bar under the top plate join (2 pieces) | `XL_deck_bar.stl` | flat, **supports off** |
| Controller case, base | `XL_controller_case_base.stl` | upright, open top up |
| Controller case, lid | `XL_controller_case_lid.stl` | top face down |
| Tray over the gear, left half (switch end) | `PACKOUT_XL_tray_switch_end_walkie_standing.stl` | standing on its middle edge, turned 45 deg; supports on build plate only, style Snug |
| Tray over the gear, right half (Anderson end) | `PACKOUT_XL_tray_anderson_end_standing.stl` | the same way |

No nuts anywhere: the screws (M5 x 10, M3 x 10, M5 x 16 for the breakers) cut their own threads in the plastic. Section 3 of the build book lists every screw.

## Not finished yet

- The Renogy COMM and REMOTE jack pin numbers aren't published yet. Leave those wires off (the wiring guide shows them dashed).
- The right tray half and the controller case haven't been printed and test-fitted yet.
