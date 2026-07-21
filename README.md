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
- **The run**: a burn→cruise→brake profile at the hardest proper acceleration
  the load's g-rating (through your dampers) tolerates — honest relativistic
  kinematics, closed-form in rapidity. Gentle ramps cost years on both clocks,
  so fragile freight is where your career actually goes. The optimizer picks
  the cruise rapidity that minimizes your aging within the deadline, the
  passenger cap, and the tank (and won't waste Δv on savings under ~3 weeks).
  Replayed as a short animation of the two clocks ticking at their true
  relative rates.
- **Fuel & outfitting**: bulk-discounted Δv and the dock's two-item upgrade
  shop (tank, drive, dampers, broker license, rejuv, Redline Coils, and a
  Docking Assist reworked for the watch: hot-dock at 0.2c, saving Δv every
  leg). Dock economy events (booms, strikes, fuel rationing) move prices.
  Stranded with a dry tank? A **recovery tow** costs 40% of your credits and
  4 years of your life.
- **Career**: reputation ramps pay, pilots retire at 82 (plus rejuv), and the
  final balance earns a rank from *Deadhead* to *The Ageless*. Touch γ 25,000
  on a clean delivery and the deep-space long-haul stations open up — which
  the ramp physics makes a real gate: only maxed dampers on rugged long-haul
  freight can build that γ inside a deadline.
- **Title screen**: continue, new career, today's shared **daily run**
  (date-seeded), and a how-to-play primer. All-time records (best balance,
  most deliveries, highest γ) persist across careers.

Since the autopilot flies each leg as fast as the fuel allows, how much fuel
you buy is the throttle. The state persists on the watch between launches.

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
