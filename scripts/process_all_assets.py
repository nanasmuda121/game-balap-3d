#!/usr/bin/env python3
import os
import math

SRC_DIR = "/storage/emulated/0/Download/src"
TARGET_DIRS = [
    "/root/game-balap-3d/assets",
    "/root/game-balap-3d/android/app/src/main/assets",
    "/storage/emulated/0/Download/game-balap-3d/assets",
    "/storage/emulated/0/Download/game-balap-3d/android/app/src/main/assets"
]

for d in TARGET_DIRS:
    os.makedirs(d, exist_ok=True)

def calc_normal(v0, v1, v2):
    ax, ay, az = v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]
    bx, by, bz = v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]
    nx = ay * bz - az * by
    ny = az * bx - ax * bz
    nz = ax * by - ay * bx
    l = math.sqrt(nx*nx + ny*ny + nz*nz)
    if l > 1e-6:
        return (nx/l, ny/l, nz/l)
    return (0.0, 1.0, 0.0)

def process_model(in_filename, out_base_name, is_car=True, body_rgb=(0.1, 0.5, 0.95)):
    in_path = os.path.join(SRC_DIR, in_filename)
    if not os.path.exists(in_path):
        print(f"Warning: {in_path} does not exist!")
        return

    with open(in_path, 'r', errors='ignore') as f:
        lines = f.readlines()

    raw_verts = []
    tri_faces = []
    curr_g = 'default'

    for line in lines:
        if line.startswith('g ') or line.startswith('o '):
            parts = line.strip().split(maxsplit=1)
            curr_g = parts[1] if len(parts) > 1 else 'unnamed'
        elif line.startswith('v '):
            parts = line.split()
            raw_verts.append((float(parts[1]), float(parts[2]), float(parts[3])))
        elif line.startswith('f '):
            # Ignore 6-face dummy cube artifact from 3D Modeling App
            if curr_g != 'name0':
                parts = line.strip().split()[1:]
                indices = [int(p.split('/')[0]) - 1 for p in parts]
                if len(indices) == 3:
                    tri_faces.append((indices[0], indices[1], indices[2]))
                elif len(indices) == 4:
                    tri_faces.append((indices[0], indices[1], indices[2]))
                    tri_faces.append((indices[0], indices[2], indices[3]))

    # Used vertices only
    used_indices = sorted(list(set(idx for face in tri_faces for idx in face)))
    remap = {old_idx: new_idx for new_idx, old_idx in enumerate(used_indices)}
    remapped_faces = [(remap[f[0]], remap[f[1]], remap[f[2]]) for f in tri_faces]

    # Normalize coordinates
    xs = [raw_verts[i][0] for i in used_indices]
    ys = [raw_verts[i][1] for i in used_indices]
    zs = [raw_verts[i][2] for i in used_indices]

    cx = (min(xs) + max(xs)) / 2.0
    min_y = min(ys)
    cz = (min(zs) + max(zs)) / 2.0

    verts = []
    for i in used_indices:
        x = raw_verts[i][0] - cx
        y = raw_verts[i][1] - min_y
        z = raw_verts[i][2] - cz
        if is_car:
            # Rotate 180 degrees so front hood faces -Z (Forward)
            verts.append((-x, y, -z))
        else:
            verts.append((x, y, z))

    # Calculate smooth vertex normals
    vert_normals = [[0.0, 0.0, 0.0] for _ in range(len(verts))]
    face_normals = []
    for f in remapped_faces:
        fn = calc_normal(verts[f[0]], verts[f[1]], verts[f[2]])
        face_normals.append(fn)
        for vi in f:
            vert_normals[vi][0] += fn[0]
            vert_normals[vi][1] += fn[1]
            vert_normals[vi][2] += fn[2]

    # Normalize vertex normals
    final_normals = []
    for vn in vert_normals:
        l = math.sqrt(vn[0]*vn[0] + vn[1]*vn[1] + vn[2]*vn[2])
        if l > 1e-6:
            final_normals.append((vn[0]/l, vn[1]/l, vn[2]/l))
        else:
            final_normals.append((0.0, 1.0, 0.0))

    # Classify faces into materials
    mat_faces = {}
    if is_car:
        # Car material buckets
        mat_faces['mat_body'] = []
        mat_faces['mat_glass'] = []
        mat_faces['mat_tires'] = []
        mat_faces['mat_rims'] = []
        mat_faces['mat_headlights'] = []
        mat_faces['mat_taillights'] = []

        for fi, f in enumerate(remapped_faces):
            p0, p1, p2 = verts[f[0]], verts[f[1]], verts[f[2]]
            cen_y = (p0[1] + p1[1] + p2[1]) / 3.0
            cen_z = (p0[2] + p1[2] + p2[2]) / 3.0
            cen_x = (p0[0] + p1[0] + p2[0]) / 3.0

            # Front Headlights
            if cen_z < -1.8 and cen_y > 0.3 and cen_y < 0.7:
                mat_faces['mat_headlights'].append(f)
            # Rear Taillights
            elif cen_z > 1.8 and cen_y > 0.4 and cen_y < 0.8:
                mat_faces['mat_taillights'].append(f)
            # Lower Wheels / Tires
            elif cen_y < 0.42 and abs(cen_x) > 0.6:
                if abs(cen_x) > 0.95:
                    mat_faces['mat_rims'].append(f)
                else:
                    mat_faces['mat_tires'].append(f)
            # Cabin Windows / Glass
            elif cen_y > 0.75 and abs(cen_z) < 1.4 and (abs(cen_x) > 0.5 or cen_y > 1.05):
                mat_faces['mat_glass'].append(f)
            else:
                mat_faces['mat_body'].append(f)
    else:
        # Building material buckets
        mat_faces['mat_walls'] = []
        mat_faces['mat_roof'] = []
        mat_faces['mat_windows'] = []
        max_y = max(v[1] for v in verts)

        for fi, f in enumerate(remapped_faces):
            cen_y = (verts[f[0]][1] + verts[f[1]][1] + verts[f[2]][1]) / 3.0
            fn = face_normals[fi]
            # Roof (upper faces with upward normal)
            if cen_y > max_y * 0.7 and fn[1] > 0.4:
                mat_faces['mat_roof'].append(f)
            # Windows (steep walls at mid-height)
            elif cen_y > max_y * 0.25 and cen_y < max_y * 0.65 and (fi % 4 == 0):
                mat_faces['mat_windows'].append(f)
            else:
                mat_faces['mat_walls'].append(f)

    # Write MTL file content
    mtl_content = f"# Materials for {out_base_name}\n\n"
    if is_car:
        r, g, b = body_rgb
        mtl_content += f"newmtl mat_body\nKa {r*0.2:.3f} {g*0.2:.3f} {b*0.2:.3f}\nKd {r:.3f} {g:.3f} {b:.3f}\nKs 0.8 0.8 0.8\nNs 80.0\nillum 2\n\n"
        mtl_content += "newmtl mat_glass\nKa 0.05 0.1 0.15\nKd 0.15 0.3 0.45\nKs 0.9 0.9 0.9\nNs 100.0\nd 0.85\nillum 2\n\n"
        mtl_content += "newmtl mat_tires\nKa 0.05 0.05 0.05\nKd 0.12 0.12 0.12\nKs 0.1 0.1 0.1\nNs 10.0\nillum 1\n\n"
        mtl_content += "newmtl mat_rims\nKa 0.2 0.2 0.2\nKd 0.75 0.75 0.75\nKs 0.9 0.9 0.9\nNs 90.0\nillum 2\n\n"
        mtl_content += "newmtl mat_headlights\nKa 0.3 0.3 0.3\nKd 1.0 1.0 1.0\nKs 1.0 1.0 1.0\nNs 120.0\nillum 2\n\n"
        mtl_content += "newmtl mat_taillights\nKa 0.2 0.02 0.02\nKd 0.9 0.1 0.1\nKs 0.8 0.3 0.3\nNs 60.0\nillum 2\n\n"
    else:
        mtl_content += "newmtl mat_walls\nKa 0.15 0.15 0.15\nKd 0.82 0.78 0.72\nKs 0.2 0.2 0.2\nNs 20.0\nillum 1\n\n"
        mtl_content += "newmtl mat_roof\nKa 0.15 0.05 0.05\nKd 0.65 0.22 0.18\nKs 0.3 0.3 0.3\nNs 30.0\nillum 1\n\n"
        mtl_content += "newmtl mat_windows\nKa 0.1 0.2 0.3\nKd 0.3 0.6 0.85\nKs 0.9 0.9 0.9\nNs 90.0\nillum 2\n\n"

    # Write OBJ file content
    obj_content = f"# Cleaned 3D Model: {out_base_name}\n"
    obj_content += f"mtllib {out_base_name}.mtl\n\n"

    # Vertices
    for v in verts:
        obj_content += f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n"

    # Normals
    for n in final_normals:
        obj_content += f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n"

    # Faces by Material
    for mat_name, faces in mat_faces.items():
        if not faces: continue
        obj_content += f"\nusemtl {mat_name}\n"
        for f in faces:
            # 1-indexed for OBJ format
            i0, i1, i2 = f[0] + 1, f[1] + 1, f[2] + 1
            obj_content += f"f {i0}//{i0} {i1}//{i1} {i2}//{i2}\n"

    # Save to all target folders
    for out_dir in TARGET_DIRS:
        obj_file = os.path.join(out_dir, f"{out_base_name}.obj")
        mtl_file = os.path.join(out_dir, f"{out_base_name}.mtl")
        with open(obj_file, 'w') as fp:
            fp.write(obj_content)
        with open(mtl_file, 'w') as fp:
            fp.write(mtl_content)

    print(f"Successfully processed: {in_filename} -> {out_base_name}.obj ({len(verts)} verts, {len(remapped_faces)} tris)")

def main():
    print("=== Processing All 3D Assets ===")
    # 1. Player Car (Electric Blue)
    process_model("mobil.obj", "player_car", is_car=True, body_rgb=(0.05, 0.45, 0.95))

    # 2. Bot Car 1 (Racing Crimson Red)
    process_model("mobil2.obj", "bot_car", is_car=True, body_rgb=(0.92, 0.15, 0.12))

    # 3. Bot Car 2 / Alternative (Cyber Gold / Yellow)
    process_model("mobil3.obj", "bot_car2", is_car=True, body_rgb=(0.98, 0.75, 0.05))

    # 4. Building 1 (Villa / Townhouse)
    process_model("rumah.obj", "building_villa", is_car=False)

    # 5. Building 2 (Urban Apartment)
    process_model("rumah(1).obj", "building_apt", is_car=False)

if __name__ == "__main__":
    main()
