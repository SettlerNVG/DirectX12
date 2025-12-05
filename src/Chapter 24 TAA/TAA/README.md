# Chapter 24: Temporal Anti-Aliasing (TAA)

## Overview

This chapter implements industry-standard Temporal Anti-Aliasing (TAA) for DirectX 12, following best practices from leading graphics engineers.

## What is TAA?

TAA is a post-processing technique that:
- Reduces aliasing (jagged edges) by accumulating samples across multiple frames
- Uses sub-pixel jitter to sample different positions within each pixel
- Reprojects previous frame data using motion vectors
- Blends current and historical frames intelligently

## Key Components

### 1. **Jitter Pattern** (`TemporalAA.cpp`)
- Halton (2,3) sequence with 8 samples
- Provides low-discrepancy sub-pixel offsets
- Applied to projection matrix each frame

### 2. **Motion Vectors** (`MotionVectors.hlsl`)
- Per-pixel velocity in texture space
- Tracks object movement between frames
- Used for history reprojection

### 3. **TAA Resolve** (`TAAResolve.hlsl`)
- Reprojects history using motion vectors
- Clips history to neighborhood variance
- Adaptively blends based on motion/variance/depth
- Applies sharpening to compensate for blur

## Features Implemented

✅ **Halton Jitter Sequence** - Optimal sub-pixel sampling pattern  
✅ **Catmull-Rom Filtering** - High-quality history sampling  
✅ **Variance-Based Clipping** - Superior ghosting reduction  
✅ **YCoCg Color Space** - Better perceptual clamping  
✅ **Velocity Dilation** - Improved edge quality  
✅ **Depth-Based Rejection** - Disocclusion detection  
✅ **Adaptive Blending** - Motion/variance-aware mixing  
✅ **Sharpening Pass** - Compensates for temporal blur  

## Usage

### Controls
- **T Key**: Toggle TAA on/off
- **WASD**: Move camera
- **Mouse**: Look around

### Quality Settings

Adjust in `UpdateTAACB()`:

```cpp
// Conservative (less ghosting)
mTAACB.BlendFactor = 0.1f;

// Balanced (default)
mTAACB.BlendFactor = 0.05f;

// Aggressive (more stable)
mTAACB.BlendFactor = 0.03f;
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     TAA Pipeline                             │
└─────────────────────────────────────────────────────────────┘

Frame N:
  ┌──────────────┐
  │ Apply Jitter │ (Halton sequence)
  └──────┬───────┘
         │
  ┌──────▼───────────────────────────────────────┐
  │ Render Scene                                  │
  │  • Scene Color Buffer (jittered projection)   │
  │  • Scene Depth Buffer                         │
  └──────┬───────────────────────────────────────┘
         │
  ┌──────▼───────────────────────────────────────┐
  │ Generate Motion Vectors                       │
  │  • Compare current vs previous positions      │
  │  • Apply 3x3 max filter (velocity dilation)   │
  └──────┬───────────────────────────────────────┘
         │
  ┌──────▼───────────────────────────────────────┐
  │ TAA Resolve Pass                              │
  │  1. Sample current frame                      │
  │  2. Reproject using motion vectors            │
  │  3. Sample history (Catmull-Rom)              │
  │  4. Check disocclusion (depth)                │
  │  5. Calculate neighborhood variance           │
  │  6. Clip history (YCoCg AABB)                 │
  │  7. Adaptive blend                            │
  │  8. Sharpen                                   │
  └──────┬───────────────────────────────────────┘
         │
  ┌──────▼───────────────────────────────────────┐
  │ Swap Buffers                                  │
  │  • Current → History for next frame           │
  └──────┬───────────────────────────────────────┘
         │
  ┌──────▼───────────────────────────────────────┐
  │ Present to Screen                             │
  └───────────────────────────────────────────────┘
```

## Performance

### GPU Cost
- **Motion Vectors**: ~0.2ms @ 1080p
- **TAA Resolve**: ~0.5-1.0ms @ 1080p (depends on Catmull-Rom)
- **Total**: ~0.7-1.2ms @ 1080p

### Memory
- **Per Pixel**: ~16 bytes
- **1080p**: ~32 MB
- **4K**: ~128 MB

## References

This implementation is based on:

1. **Eduardo Lopez** - [Temporal AA and the Quest for the Holy Trail](https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/)
   - Variance-based clipping
   - YCoCg color space
   - Sharpening techniques

2. **Sugulee** - [Temporal Anti-Aliasing Tutorial](https://sugulee.wordpress.com/2021/06/21/temporal-anti-aliasingtaa-tutorial/)
   - Catmull-Rom filtering
   - Neighborhood clamping
   - Implementation details

3. **Alex Tardif** - [TAA](https://alextardif.com/TAA.html)
   - Velocity dilation
   - Depth-based rejection
   - AABB clipping algorithm

## Files

### Core Implementation
- `TemporalAA.h/cpp` - TAA buffer management and jitter generation
- `MotionVectors.h/cpp` - Motion vector buffer management
- `TAAApp.cpp` - Main application with TAA integration
- `FrameResource.h/cpp` - Per-frame constant buffers

### Shaders
- `TAAResolve.hlsl` - Main TAA resolve shader
- `MotionVectors.hlsl` - Motion vector generation
- `Default.hlsl` - Scene rendering with jitter
- `Common.hlsl` - Shared shader code

### Documentation
- `TAA_Implementation_Notes.md` - Detailed technical documentation
- `CHANGES.md` - Summary of all changes and fixes
- `README.md` - This file

## Troubleshooting

### Ghosting Trails
- Increase blend factor (0.05 → 0.1)
- Check motion vector accuracy
- Verify depth-based rejection is working

### Too Much Aliasing
- Decrease blend factor (0.05 → 0.03)
- Verify jitter is being applied correctly
- Check Halton sequence implementation

### Blurry Image
- Increase sharpening amount
- Verify Catmull-Rom filtering is active
- Check if variance clipping is too aggressive

### Flickering
- Check for NaN/Inf values in shaders
- Verify history buffer initialization
- Ensure proper buffer swapping

## Building

This project is part of the DirectX 12 book samples. Build using:

```bash
# Open solution
start src/AllProjects.sln

# Or build specific project
msbuild "src/Chapter 24 TAA/TAA/TAA.vcxproj"
```

## License

Part of "Introduction to 3D Game Programming with DirectX 12" sample code.
