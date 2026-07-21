# Lighthaul for Pebble

*The relativistic courier game, on your wrist.* A Pebble Time 2 port of
[Lighthaul](../Lighthaul) that keeps the economy and drops the joystick: pick
contracts off a star chart, outfit the ship, and let the autopilot fly the run
optimally — then watch the two clocks disagree about how long it took.

## The game

You dock at a station on a procedurally generated cluster (seed-compatible
with the web game's worldgen — the same seed names the same stations).

- **Star map** (docked screen): Up/Down cycles the three contract offers plus
  FUEL & OUTFITTING; the gold route shows the selected leg, with pay, g-rating,
  distance, deadline, and a live OK/!! feasibility check. Select opens it.
- **Contract card**: the deal on top, the auto-computed flight plan below —
  cruise β and γ, Δv burned vs. tank, universe-years and ship-years, and what
  age you'll be on arrival. Select accepts and flies.
- **The run**: resolved instantly with the real clock math (deadlines on
  universe time `d/β`, aging on ship time `d/βγ`, fuel as rapidity paid again
  to brake), replayed as a short animation of the two clocks ticking at their
  true relative rates.
- **Fuel & outfitting**: bulk-discounted Δv and the dock's two-item upgrade
  shop (tank, drive, dampers, broker license, rejuv, Redline Coils, docking
  assist). Dock economy events (booms, strikes, fuel rationing) move prices.
- **Career**: reputation ramps pay, pilots retire at 82 (plus rejuv), and the
  final balance earns a rank from *Deadhead* to *The Ageless*. Touch γ 25,000
  on a clean delivery and the deep-space long-haul stations open up.

The flight plan burns the whole usable tank — pilot time is the real currency,
so speed is controlled by how much fuel you choose to buy. The state persists
on the watch between launches.

## Building & running

```sh
pebble build
pebble install --emulator emery       # Pebble Time 2 emulator
pebble install --phone <ip>           # sideload via the Pebble app
```

## Layout

- `src/c/world.{c,h}` — seeded worldgen: cluster layout, names, shops, contracts.
- `src/c/game.{c,h}` — economy, flight planning/resolution, persistence.
- `src/c/rng.{c,h}` — mulberry32 + FNV-1a, matching the web game.
- `src/c/fastmath.{c,h}` — double `sqrt/exp/ln/tanh/atanh`. The SDK's libm
  faults at runtime on real hardware (`sqrtf` included — QEMU tolerates calls
  the watch does not), so no source file may include `<math.h>`.
- `src/c/win_*.c` — map, contract, flight, results, outfitting windows.
