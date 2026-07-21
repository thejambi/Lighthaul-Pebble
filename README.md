# Lighthaul for Pebble

*The relativistic courier game, on your wrist.* A Pebble port of
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
  the load's g-rating (through your dampers) tolerates — capped by the drive's
  own 7g thrust — honest relativistic kinematics, closed-form in rapidity.
  Gentle ramps cost years on both clocks, so fragile freight is where your
  career actually goes. Replayed as a short animation of the two clocks
  ticking at their true relative rates.
- **The speed ladder**: Up/Down on the contract card cycles the cruise plan —
  AUTO (least aging within deadline, cap, and tank), fixed rungs from 0.5c to
  0.999999c, and MAX (whatever the governor allows — Redline Coils raise it). Every rung shows its Δv, both clocks, and your arrival age live: dial
  down to save fuel at the cost of your years, dial up to buy youth with Δv.
  The pick is sticky between flights.
- **Fuel & outfitting**: bulk-discounted Δv and the dock's two-item upgrade
  shop (tank, drive, dampers, broker license, rejuv, Redline Coils, and a
  Docking Assist reworked for the watch: hot-dock at 0.2c, saving Δv every
  leg). Dock economy events (booms, strikes, fuel rationing) move prices.
  Stranded broke with a dry tank? A **recovery tow** appears only when you
  can't afford market fuel — its real price is 4 years of your life.
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
pebble build                          # all six platforms
pebble install --emulator emery       # or basalt/chalk/diorite/flint/gabbro
pebble install --phone                # sideload via the Pebble app
```

Runs on every modern Pebble: Time/Steel (basalt), Time Round (chalk),
Pebble 2 (diorite), Time 2 (emery), Core 2 Duo (flint), and Round 2 (gabbro).
Layouts adapt per screen (roomy 200x228, compact 144x168, round with insets);
black & white watches carry all state in text and shape, never color alone.
The original Pebble (aplite) doesn't fit in its 24K app region.

Published on the Pebble appstore:
https://apps.repebble.com/9420a2e218e54ce986c7c871

## Layout

- `src/c/world.{c,h}` — seeded worldgen: cluster layout, names, shops, contracts.
- `src/c/game.{c,h}` — economy, flight planning/resolution, persistence.
- `src/c/rng.{c,h}` — mulberry32 + FNV-1a, matching the web game.
- `src/c/fastmath.{c,h}` — double `sqrt/exp/ln/tanh/atanh`. The SDK's libm
  faults at runtime on real hardware (`sqrtf` included — QEMU tolerates calls
  the watch does not), so no source file may include `<math.h>`.
- `src/c/win_*.c` — map, contract, flight, results, outfitting windows.
