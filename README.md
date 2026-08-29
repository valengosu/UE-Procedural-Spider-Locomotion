## Demo

[![Procedural Spider Locomotion Demo](https://img.youtube.com/vi/YJkYJigPHQY/maxresdefault.jpg)](https://youtu.be/YJkYJigPHQY)

A procedural multi-legged locomotion system built in Unreal Engine, focused on stable foot placement and separation between locomotion logic and animation execution.

## Engineering Problem

Procedural multi-legged movement requires each foot to remain stable while planted, adapt to uneven terrain, and determine when and where to take the next step without coupling locomotion decisions directly to animation.

## Key Design Decisions

**Independent leg state**  
Each leg maintains its own movement state and foot target, allowing individual legs to transition between planted and stepping states independently.

**Multi-stage surface detection**  
Multiple traces are used to find and validate suitable foot placement targets across uneven surfaces.

**Locomotion planning separated from animation**  
The C++ component determines when and where each leg moves, while Control Rig applies the resulting targets through IK.

## Technical Highlights

- Independent per-leg locomotion state
- Runtime foot target generation
- Multi-stage surface detection
- Stable procedural foot placement
- C++ locomotion logic + Control Rig IK
