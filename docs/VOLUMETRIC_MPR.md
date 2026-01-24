# Volumetric Multi-Planar Reconstruction (MPR) Specification

This document details the mathematical and geometrical foundations of the Voxel Multi-Planar Reconstruction (MPR) and Ray-Cast Tomography Resampling Engine.

---

## 1. Patient Coordinate System (RCS)

Per DICOM PS 3.3 Section C.7.6.2.1, spatial coordinates are expressed in the Reference Coordinate System (RCS), a right-handed orthogonal Cartesian coordinate system:
- $+X$ points towards the Patient's Left.
- $+Y$ points towards the Patient's Posterior (back).
- $+Z$ points towards the Patient's Superior (head).

### 1.1 Orientation Cosines
Each 2D image slice provides the Image Orientation (Patient) tag `(0020,0037)`, consisting of six direction cosines:
$$\mathbf{X} = \begin{pmatrix} X_x \\ X_y \\ X_z \end{pmatrix}, \quad \mathbf{Y} = \begin{pmatrix} Y_x \\ Y_y \\ Y_z \end{pmatrix}$$

Where:
- $\mathbf{X}$ is the unit direction vector of the first row with respect to the patient.
- $\mathbf{Y}$ is the unit direction vector of the first column with respect to the patient.

The slice normal vector $\mathbf{Z}$ is computed by the cross product:
$$\mathbf{Z} = \mathbf{X} \times \mathbf{Y} = \begin{pmatrix} X_y Y_z - X_z Y_y \\ X_z Y_x - X_x Y_z \\ X_x Y_y - X_y Y_x \end{pmatrix}$$

### 1.2 Slice Sorting & Grid Assembly
Given a set of $N$ slices with Image Position (Patient) coordinates $\mathbf{S}_i = (S_{x,i}, S_{y,i}, S_{z,i})^T$ (`0020,0032`), the scalar projection distance along the slice normal is:
$$d_i = \mathbf{S}_i \cdot \mathbf{Z}$$

Slices are sorted in ascending order of $d_i$. The slice spacing $\Delta z$ is given by:
$$\Delta z = \frac{1}{N - 1} \sum_{i=1}^{N-1} (d_i - d_{i-1})$$

A contiguous 3D discrete regular scalar field $V(x, y, z)$ is constructed with dimensions $N_x \times N_y \times N_z$ and voxel spacings $(\Delta x, \Delta y, \Delta z)$.

---

## 2. Sub-Voxel Trilinear Interpolation

When sampling the volume at non-integer coordinates $(x, y, z) \in \mathbb{R}^3$, nearest-neighbor sampling produces severe stair-stepping (aliasing) artifacts. Voxel uses 3D trilinear interpolation to reconstruct smooth, continuous attenuation fields.

### 2.1 Interpolation Weights
Let:
$$x_0 = \lfloor x \rfloor, \quad x_1 = x_0 + 1, \quad u = x - x_0$$
$$y_0 = \lfloor y \rfloor, \quad y_1 = y_0 + 1, \quad v = y - y_0$$
$$z_0 = \lfloor z \rfloor, \quad z_1 = z_0 + 1, \quad w = z - z_0$$

With clamp boundaries:
$$x_0, x_1 \in [0, N_x - 1]$$
$$y_0, y_1 \in [0, N_y - 1]$$
$$z_0, z_1 \in [0, N_z - 1]$$

The continuous value $f(x, y, z)$ is computed as the weighted sum of the 8 surrounding corner voxels:
$$f(x, y, z) = (1 - u)(1 - v)(1 - w) V_{000} + u(1 - v)(1 - w) V_{100} + (1 - u)v(1 - w) V_{010} + uv(1 - w) V_{110} + (1 - u)(1 - v)w V_{001} + u(1 - v)w V_{101} + (1 - u)vw V_{011} + uvw V_{111}$$

Where $V_{ijk} = V(x_i, y_j, z_k)$.

---

## 3. Orthogonal Multi-Planar Reconstruction

MPR generates synthetic 2D planar cross-sections through the 3D volume grid along the three primary anatomical planes:

```
          Superior (+Z)
              ^
              |   Coronal (XZ)
              |  /
              | /
              |/
<-------------+-------------> Left (+X)
 Posterior   /|  Axial (XY)
   (+Y)     / |
           /  |
          v   |
             Inferior (-Z)
          Sagittal (YZ)
```

### 3.1 Axial (Transverse Plane)
- Preserves the native imaging orientation (XY plane).
- Fixed coordinate: $z = k \in [0, N_z - 1]$.
- Output resolution: $N_x \times N_y$ pixels.
- Pixel value: $I(x, y) = V(x, y, k)$.

### 3.2 Coronal (Frontal Plane)
- Slices the volume from Anterior (front) to Posterior (back).
- Fixed coordinate: $y = j \in [0, N_y - 1]$.
- Output resolution: $N_x \times N_z$ pixels.
- Spatial aspect ratio correction: each output pixel $(x', z')$ corresponds to physical position $(x' \cdot \Delta x, z' \cdot \Delta z)$.
- Resampled value: $I(x, z) = f(x, j, z)$.

### 3.3 Sagittal (Lateral Plane)
- Slices the volume from Patient Right to Patient Left.
- Fixed coordinate: $x = i \in [0, N_x - 1]$.
- Output resolution: $N_y \times N_z$ pixels.
- Resampled value: $I(y, z) = f(i, y, z)$.

---

## 4. Ray-Cast Intensity Projections

For thick-slab visualization and angiographic vessel tracing, Voxel provides three ray-casting projection operators:

### 4.1 Maximum Intensity Projection (MIP)
Evaluates the maximum radiodensity along the projection ray:
$$I_{\text{MIP}}(u, v) = \max_{k \in [0, K-1]} V(\mathbf{P}_0 + k \cdot \Delta \mathbf{P})$$
Clinical use: Highlighting contrast-enhanced blood vessels, pulmonary nodules, and calcified structures.

### 4.2 Minimum Intensity Projection (MinIP)
Evaluates the minimum radiodensity along the projection ray:
$$I_{\text{MinIP}}(u, v) = \min_{k \in [0, K-1]} V(\mathbf{P}_0 + k \cdot \Delta \mathbf{P})$$
Clinical use: Visualizing low-density air spaces, tracheobronchial tree, and emphysema.

### 4.3 Average Intensity Projection (AIP)
Computes the mean attenuation across the slab:
$$I_{\text{AIP}}(u, v) = \frac{1}{K} \sum_{k=0}^{K-1} V(\mathbf{P}_0 + k \cdot \Delta \mathbf{P})$$
Clinical use: Emulating conventional plain-film radiographs.

---

## 5. Hounsfield Unit Scaling & VOI Window/Level LUT

### 5.1 Hounsfield Unit Transformation
Raw 16-bit stored pixel values $P$ are transformed into calibrated Hounsfield Units (HU) via:
$$\text{HU} = P \times \text{RescaleSlope} + \text{RescaleIntercept}$$

Standard CT Hounsfield scale reference points:
- Air: $-1000$ HU
- Lung: $-600$ to $-400$ HU
- Fat: $-100$ to $-50$ HU
- Water: $0$ HU
- Cerebrospinal Fluid (CSF): $+15$ HU
- Brain Gray Matter: $+35$ to $+45$ HU
- Acute Blood: $+60$ to $+80$ HU
- Cortical Bone: $+1000$ to $+3000$ HU

### 5.2 Linear VOI Contrast Transfer Function
Per DICOM PS 3.3 Section C.11.2.1.2, display grayscale value $y \in [0, 255]$ for window center $c$ and window width $w$ is defined as:
$$y = \begin{cases} 0 & x \le c - 0.5 - \frac{w - 1}{2} \\ \left\lfloor \frac{x - (c - 0.5)}{w - 1} + 0.5 \right\rfloor \times 255 & c - 0.5 - \frac{w - 1}{2} < x \le c - 0.5 + \frac{w - 1}{2} \\ 255 & x > c - 0.5 + \frac{w - 1}{2} \end{cases}$$

### 5.3 Clinical Window/Level Presets
| Preset Name | Window Center ($c$) | Window Width ($w$) | Target Anatomy |
|:---|:---:|:---:|:---|
| **Soft Tissue** | $+40$ HU | $350$ HU | Abdomen, muscles, lymph nodes |
| **Bone** | $+400$ HU | $2000$ HU | Skeletal fractures, cortical detail |
| **Lung** | $-600$ HU | $1500$ HU | Bronchial tree, parenchyma |
| **Brain** | $+40$ HU | $80$ HU | Ischemic stroke, gray/white contrast |
| **Liver** | $+30$ HU | $150$ HU | Hepatic lesions, steatosis |
| **Mediastinum** | $+50$ HU | $450$ HU | Great vessels, cardiac silhouette |
