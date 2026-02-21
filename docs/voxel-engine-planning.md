# Voxel Engine / Game Master Plan

## Purpose

This document is the long-lived implementation roadmap for the voxel engine/game initiative in this repository. It is written to support:

- Multi-developer collaboration
- Multi-session agentic execution
- Stable architectural decisions over long timelines
- Incremental delivery with measurable checkpoints

The project goal is a reusable voxel engine/framework that can power this game and potentially other projects, while preserving the current top-level launcher/state system.

---

## Confirmed Decisions (as of 2026-02-21)

These are locked unless intentionally revised with an ADR update:

1. **Physics engine direction:** Jolt-first
2. **World storage baseline:** dense chunked grid first, `32^3` chunks
3. **Spatial hierarchy strategy:** dense chunks as source of truth; sparse acceleration structure added later
4. **ECS scope:** gameplay-state local registry/runtime (destroy on gameplay state exit)
5. **Dependency strategy:** EnTT vendored/header-only + Jolt submodule
6. **Top-level app flow:** keep existing selector/launcher pattern minimal and unchanged

---

## Non-Goals (for early phases)

To protect the foundation, these are intentionally out of scope for first milestones:

- Full multiplayer/netcode
- Fully dynamic voxel destruction simulation everywhere (Teardown-like)
- Complex liquids with full fluid simulation
- Large-scale mod/plugin ABI guarantees
- Extensive UI polish beyond debug/gameplay-critical tooling

---

## Architectural Principles

1. **Lifecycle clarity over convenience**
   - Gameplay runtime owns its ECS/world/physics/job resources.
   - Entering gameplay creates runtime; leaving gameplay destroys it.

2. **Data-oriented world core**
   - Voxel world storage remains outside ECS hot loops.
   - ECS handles dynamic entities, behavior, and orchestration.

3. **Main-thread rendering boundaries**
   - OpenGL work remains on the main thread.
   - Worker threads only perform CPU-side generation and meshing.

4. **Stable interfaces, replaceable internals**
   - Build narrow interfaces around physics, world queries, meshing, and streaming.
   - Keep room for future sparse acceleration without rewriting gameplay systems.

5. **Incremental vertical slices**
   - Deliver full pipeline slices early (generate -> mesh -> render -> interact) before advanced features.

---

## High-Level Runtime Model

### Runtime Ownership

- **Top-level state manager** (already present): selects projects/states.
- **Voxel gameplay state**: owns a gameplay runtime package containing:
  - World/chunk storage
  - Streaming subsystem
  - Meshing pipeline
  - ECS registry + systems
  - Physics world integration
  - Save/load services

### Frame/Simulation Timing

Inside gameplay state:

- Use **fixed timestep** for simulation (physics + gameplay systems)
- Use render/update interpolation for smooth visuals
- Keep state-level update deterministic where practical

### Data Domains

- **World domain (non-ECS):** chunk storage, block IDs/materials, chunk metadata
- **Entity domain (ECS):** dynamic objects (player, mobs, items, projectiles, transport)
- **Bridge domain:** components/handles referencing physics bodies and world positions

---

## Proposed Module Layout (first-party)

> Exact file names may evolve; preserve this layering.

- `include/sandbox/voxel/`
  - `world/` (chunk types, coordinates, storage API)
  - `streaming/` (residency and queues)
  - `meshing/` (mesh build interfaces)
  - `physics/` (Jolt adapter interfaces)
  - `ecs/` (components/system declarations)
  - `runtime/` (voxel runtime orchestration)
  - `io/` (serialization interfaces)
- `src/sandbox/voxel/`
  - concrete implementations mirroring include layout
- `src/sandbox/states/`
  - `voxel_game_state.*`

---

## Milestone Roadmap

## M0 — Planning + Scaffolding

**Goal:** establish execution structure and skeleton integration.

Deliverables:

- This planning document exists and is linked from README
- New gameplay state scaffold for voxel project
- Basic runtime shell (empty subsystems with lifecycle hooks)
- Build system hooks for voxel modules (no heavy features yet)

Exit Criteria:

- App builds cleanly
- Selector can enter/exit voxel state without resource leaks/crashes

---

## M1 — World Core (Dense Chunks)

**Goal:** robust chunked voxel storage and addressing.

Deliverables:

- `ChunkKey`, world/chunk coordinate transforms
- `32^3` dense chunk block storage with cache-friendly contiguous layout
- Block/material ID model and palette conventions
- Neighbor lookup API and boundary-safe voxel reads/writes
- Dirty flags for mesh rebuild scheduling

Exit Criteria:

- Deterministic world read/write behavior
- Chunk operations covered by targeted tests (if test harness added) or deterministic debug checks

---

## M2 — Streaming + Generation Jobs

**Goal:** asynchronous terrain population and chunk residency.

Deliverables:

- Residency policy around player focus position/radius
- Job queues for generation and chunk lifecycle transitions
- Deterministic seed-based procedural generation baseline
- Main-thread safe handoff from generated data into active world storage

Exit Criteria:

- Player movement causes stable load/unload behavior
- No OpenGL calls or unsafe world mutation from worker threads

---

## M3 — Meshing + Rendering Pipeline

**Goal:** visible voxel terrain with practical performance baseline.

Deliverables:

- Face-culling chunk mesher (naive greedy deferred)
- Dirty-chunk remesh scheduling
- Main-thread GPU upload queue
- Frustum culling at chunk bounds level
- Debug visualization counters (chunk count, mesh queue depth)

Exit Criteria:

- Terrain visible, updateable, and cullable
- Frame time remains bounded in target local scenarios

---

## M4 — ECS Foundation (EnTT)

**Goal:** establish dynamic entity architecture without polluting world storage model.

Deliverables:

- Gameplay-local EnTT registry lifetime owned by voxel state
- Core components:
  - `Transform`
  - `Velocity`
  - `Renderable`
  - `ColliderRef` / `PhysicsBodyRef`
  - `Lifetime` / tags as needed
- System ordering and tick stages (input -> sim -> post-sim -> render prep)

Exit Criteria:

- Dynamic entities can be created/despawned and rendered
- ECS teardown is clean on gameplay state exit

---

## M5 — Physics Integration (Jolt)

**Goal:** stable entity physics interacting with static voxel world.

Deliverables:

- Jolt world bootstrap + adapter boundaries
- Physics body creation/synchronization from ECS components
- Voxel collision query bridge (entity/world)
- Gravity, grounded checks, and simple collision response
- Projectiles and dropped-item basic physics behavior

Exit Criteria:

- Core gameplay entities interact physically with terrain predictably
- Fixed-step simulation stable over long sessions

---

## M6 — Interaction Slice (Harvest/Fall Prototype)

**Goal:** validate event-driven gameplay architecture with real mechanics.

Deliverables:

- Block interaction events (harvest/place/remove)
- Chunk mutation propagation to meshing + physics update hooks
- Prototype rule: detect unsupported tree and trigger falling-tree behavior
- Dropped item spawn/collection loop

Exit Criteria:

- End-to-end gameplay loop works and remains maintainable
- Event pipeline avoids direct hard-coupling between systems

---

## M7 — Persistence v1

**Goal:** local save/load stability.

Deliverables:

- World metadata format (seed, version, settings)
- Chunk serialization/deserialization format
- Save boundaries integrated with gameplay runtime lifecycle
- Basic migration/versioning placeholders

Exit Criteria:

- Save -> exit -> reload preserves world changes reliably

---

## M8 — Sparse Acceleration + LOD Path

**Goal:** scalability improvements without replacing chunk source-of-truth model.

Deliverables:

- Sparse acceleration structure for visibility/range queries
- LOD strategy for far terrain representation
- Optional broadphase hints for physics region activation
- Profiling-based tuning of traversal and cache behavior

Exit Criteria:

- Large-world traversal scales better than dense-only baseline
- Existing gameplay logic remains unchanged at interface level

---

## Physics Strategy Notes (Jolt-first)

Why Jolt-first is reasonable here:

- Modern C++ API style aligns with project direction
- Strong performance reputation for game workloads
- Good fit for character + rigidbody interactions

Guardrails:

- Keep a thin `voxel::physics` adapter layer to avoid global engine coupling
- Do not expose raw Jolt types broadly outside physics module
- Use deterministic fixed step and explicit sync points with ECS

Potential deferred topics:

- Continuous collision detection for high-speed projectiles
- Advanced constraints/vehicles/boats
- Physics-driven destruction envelopes

---

## ECS Boundaries

Use ECS for:

- Dynamic entities and behavior systems
- Runtime event subscribers
- Gameplay interaction orchestration

Avoid ECS for:

- Voxel cell storage
- Chunk residency maps
- Raw meshing buffers as authoritative world data

Rationale:

- Protect cache locality for voxel queries
- Avoid over-modeling high-volume static data in component tables

---

## Threading Model (Target)

Worker threads may do:

- Procedural generation
- Mesh construction (CPU-side)
- Non-GL preprocessing

Main thread must do:

- OpenGL resource creation/upload/destruction
- Final world state commit points requiring render synchronization

Rules:

- No GL calls from worker threads
- Use explicit queue handoffs between stages
- Prefer immutable job inputs and structured outputs

---

## Performance Guardrails

Track at minimum:

- Chunk counts (active, queued generate, queued mesh, queued upload)
- Frame budget split:
  - simulation
  - generation commit
  - mesh upload
  - rendering
- Entity and physics body counts
- Worst-frame and moving average timings

Instrumentation should route through project logging facade (`LOG_*`) and/or dedicated debug UI overlays.

---

## Testing and Verification Strategy

Given the current repository has no first-party CTest suite, validation should proceed in layers:

1. **Build validation** each milestone
2. **Runtime smoke checks** (state enter/exit, core interaction loop)
3. **Deterministic scenario checks** (same seed/input -> same observable results within tolerance)
4. **Regression scenes** for chunk streaming/physics interactions

When tests are introduced, prioritize:

- Coordinate and chunk indexing correctness
- Serialization round-trip integrity
- World query edge cases at chunk boundaries

---

## Dependency Integration Plan

- EnTT: vendor/header-only under `third_party/` (or equivalent include path)
- Jolt: add as git submodule under `third_party/` and wire via target-based CMake
- Keep third-party warning noise isolated from first-party warnings
- Preserve repository’s existing CMake style and C++23 configuration

---

## ADR Protocol (for long-running project)

When revising major choices, append an ADR section at the end of this document:

- **ADR-ID:** incremental identifier (e.g., `ADR-001`)
- **Date**
- **Decision**
- **Context**
- **Alternatives considered**
- **Consequences**
- **Status:** proposed / accepted / superseded

This keeps future agent sessions aligned and prevents architectural drift.

---

## Session Handoff Checklist (for future agents/devs)

Before implementing a new task:

1. Confirm current milestone and unresolved dependencies
2. Confirm any newer ADR entries override earlier assumptions
3. Keep changes scoped to one subsystem where possible
4. Build after meaningful increments
5. Update this plan with milestone progress and changed assumptions

After completing a task:

- Record what milestone item moved forward
- Note follow-up tasks and risks discovered
- Avoid mixing broad refactors with feature implementation unless required

---

## Immediate Next Implementation Steps

These are the recommended first coding tasks after this document lands:

1. Add `voxel_game_state` skeleton and state-manager registration
2. Add voxel runtime shell (empty module interfaces + lifecycle)
3. Add CMake source groups/targets for voxel modules
4. Build and verify clean state enter/exit lifecycle
5. Add first ADR entries only if assumptions change

---

## Progress Tracking Template

Use this section as lightweight status tracking:

- **Current milestone:** `M3`
- **Completed items:**
  - Planning doc created
  - Voxel gameplay state scaffold added and wired in selector/factory
  - Voxel runtime shell added with clean state lifecycle ownership
  - M1 dense world core (`32^3` chunk representation and world access APIs)
- **In progress:**
  - M3 meshing pipeline scaffold (dirty chunks -> worker mesh build -> main-thread upload queue)
  - Meshing metadata path for future non-full and translucent blocks (water/slabs/grass/glass)
  - Render-pass bucket preparation for opaque/cutout/translucent chunk draw ordering
  - Frustum-ready chunk visibility query over pass buckets (distance cull + plane hook)
  - First on-screen real chunk mesh rendering path from uploaded pass meshes
  - Fast-exit worker teardown hardening (drop pending generation/meshing queues on shutdown)
- **Next:**
  - Replace placeholder color-only shading with material/texture-aware voxel surface shading
- **Known risks:**
  - No generic job system exists yet; will need careful design in early milestones

---

## Notes

- The top-layer selector exists as framework launcher and should remain lightweight.
- If this project is later released standalone, voxel gameplay state can become the initial app state while preserving internal architecture.
- Keep this document updated whenever milestone scope, architecture boundaries, or dependency strategy changes.
