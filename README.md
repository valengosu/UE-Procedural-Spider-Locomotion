## Demo

[![Procedural Spider Locomotion Demo](https://img.youtube.com/vi/YJkYJigPHQY/maxresdefault.jpg)](https://youtu.be/YJkYJigPHQY)

## Technical Highlights

- Implemented a reusable SpiderLegIKComponent for runtime locomotion logic.
- Used multi-stage sweep and line traces to validate landing positions.
- Separated gameplay leg planning from Control Rig animation execution.
- Managed per-leg movement through independent runtime state machines.
