# Docs

This folder contains project documentation and competition reference material.

---

## Files in This Folder

```
DEVLOG.md                          ← Full development log written for Terra Hack Club.
                                     11 entries covering every major decision, problem,
                                     and lesson learned during the build.

Poster_Rulebook.pdf                ← Official competition poster and rulebook.
                                     Contains ring dimensions, weight/size limits,
                                     match rules, and scoring system.

Sumo Robot Competition Handbook    ← Detailed competition handbook V1.
V1.pdf                               Full technical regulations, safety rules,
                                     inspection checklist, and match procedures.
```

---

## Dev Log Summary

The DEVLOG.md covers:

| Entry | Topic |
|-------|-------|
| 1 | The idea — why Pi Zero 2 W, what the goal was |
| 2 | Power architecture — single-cell tap, MT3608, capacitor fix |
| 3 | Code architecture — why 5 separate files |
| 4 | The FSM — priority ordering, heading correction |
| 5 | OpponentTracker — confidence scoring, bearing estimation |
| 6 | Adaptive tactic system — RUSHER / STALKER / FLANKER |
| 7 | Hardware watchdog — auto-recovery from crashes |
| 8 | Arduino version — Matador technique, closing speed detection |
| 9 | 3D chassis — OpenSCAD parametric design |
| 10 | Manual Bluetooth control — Dabble app, tank-drive mixing |
| 11 | Bugs — every bug hit and how it was fixed |

---

## Competition Rules Quick Reference

From the rulebook:

| Rule | Value |
|------|-------|
| Weight limit | ≤ 3 kg |
| Size limit | ≤ 20 × 20 cm |
| Height limit | No limit |
| Arena | Black circular dohyo, white boundary ring |
| Start delay | 5 seconds mandatory after signal |
| Match duration | 3 minutes (or first push-out wins) |
| Autonomous class | No remote control allowed once match starts |
