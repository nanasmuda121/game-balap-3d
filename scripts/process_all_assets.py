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

def process_model(in_filename, out_base_name, is_car=True, body_rgb=(0.88, 0.12, 0.10)):
    in_path = os.path.join(SRC_DIR, in_filename)
    if not os.path.exists(in_path):
        print(f"Error: {in_path} missing")
        return

    with open(in_path, 'r', errors='ignore') as f:
        lines = f.readlines()

    raw_verts = []
    tri_faces = []
    curr_g = ''

    for line in lines:
        if line.startswith('g ') or line.startswith('o '):
            parts = line.strip().split(maxsplit=1)
            curr_g = parts[1] if len(parts) > 1 else ''
        elif line.startswith('v '):
            p = line.split()
            raw_verts.append((float(p[1]), float(p[2]), float(p[3])))
        elif line.startswith('f '):
            # Skip dummy cube artifact in name0
            if curr_g != 'name0':
                parts = line.strip().split()[1:]
                idx = [int(p.split('/')[0]) - 1 for p in parts]
                if len(idx) == 3:
                    tri_faces.append((idx[0], idx[1], idx[2]))
                elif len(idx) == 4:
                    tri_faces.append((idx[0], idx[1], idx[2]))
                    tri_faces.append((idx[0], idx[2], idx[3]))

    used_idx = sorted(list(set(i for f in tri_faces for i in f)))
    remap = {old: new for new, old in enumerate(used_idx)}
    faces = [(remap[f[0]], remap[f[1]], remap[f[2]]) for f in tri_faces]

    xs = [raw_verts[i][0] for i in used_idx]
    ys = [raw_verts[i][1] for i in used_idx]
    zs = [raw_verts[i][2] for i in used_idx]

    cx = (min(xs) + max(xs)) / 2.0
    min_y = min(ys)
    cz = (min(zs) + max(zs)) / 2.0

    # Retain authentic orientation: Front is -Z, Rear is +Z. Do NOT rotate 180!
    verts = [(raw_verts[i][0] - cx, raw_verts[i][1] - min_y, raw_verts[i][2] - cz) for i in used_idx]

    # Calculate smooth vertex normals
    vert_normals = [[0.0, 0.0, 0.0] for _ in range(len(verts))]
    face_normals = []
    for f in faces:
        fn = calc_normal(verts[f[0]], verts[f[1]], verts[f[2]])
        face_normals.append(fn)
        for vi in f:
            vert_normals[vi][0] += fn[0]
            vert_normals[vi][1] += fn[1]
            vert_normals[vi][2] += fn[2]

    final_normals = []
    for vn in vert_normals:
        l = math.sqrt(vn[0]*vn[0] + vn[1]*vn[1] + vn[2]*vn[2])
        if l > 1e-6:
            final_normals.append((vn[0]/l, vn[1]/l, vn[2]/l))
        else:
            final_normals.append((0.0, 1.0, 0.0))

    mat_faces = {}
    if is_car:
        mat_faces['mat_body'] = []
        mat_faces['mat_glass'] = []
        mat_faces['mat_tires'] = []
        mat_faces['mat_rims'] = []
        mat_faces['mat_headlights'] = []
        mat_faces['mat_taillights'] = []

        for fi, f in enumerate(faces):
            p0, p1, p2 = verts[f[0]], verts[f[1]], verts[f[2]]
            cen_y = (p0[1] + p1[1] + p2[1]) / 3.0
            cen_z = (p0[2] + p1[2] + p2[2]) / 3.0
            cen_x = (p0[0] + p1[0] + p2[0]) / 3.0

            # Front Headlights (Z < -1.5)
            if cen_z < -1.5 and 0.3 < cen_y < 0.75:
                mat_faces['mat_headlights'].append(f)
            # Rear Taillights (Z > 1.5)
            elif cen_z > 1.5 and 0.35 < cen_y < 0.85:
                mat_faces['mat_taillights'].append(f)
            # Wheels / Tires
            elif cen_y < 0.45 and abs(cen_x) > 0.65:
                if abs(cen_x) > 0.95:
                    mat_faces['mat_rims'].append(f)
                else:
                    mat_faces['mat_tires'].append(f)
            # Cabin Windows
            elif cen_y > 0.75 and abs(cen_z) < 1.3:
                mat_faces['mat_glass'].append(f)
            else:
                mat_faces['mat_body'].append(f)
    else:
        mat_faces['mat_walls'] = []
        mat_faces['mat_roof'] = []
        mat_faces['mat_windows'] = []
        max_y = max(v[1] for v in verts)
        for fi, f in enumerate(faces):
            cen_y = (verts[f[0]][1] + verts[f[1]][1] + verts[f[2]][1]) / 3.0
            fn = face_normals[fi]
            if cen_y > max_y * 0.65 and fn[1] > 0.35:
                mat_faces['mat_roof'].append(f)
            elif cen_y > max_y * 0.2 and cen_y < max_y * 0.7 and (fi % 3 == 0):
                mat_faces['mat_windows'].append(f)
            else:
                mat_faces['mat_walls'].append(f)

    # Write MTL content
    mtl_content = f"# Material file for {out_base_name}\n\n"
    if is_car:
        r, g, b = body_rgb
        mtl_content += f"newmtl mat_body\nKa {r*0.2:.3f} {g*0.2:.3f} {b*0.2:.3f}\nKd {r:.3f} {g:.3f} {b:.3f}\nKs 0.6 0.6 0.6\nNs 60.0\nillum 2\n\n"
        mtl_content += "newmtl mat_glass\nKa 0.05 0.1 0.15\nKd 0.15 0.3 0.45\nKs 0.9 0.9 0.9\nNs 100.0\nd 0.85\nillum 2\n\n"
        mtl_content += "newmtl mat_tires\nKa 0.05 0.05 0.05\nKd 0.12 0.12 0.12\nKs 0.1 0.1 0.1\nNs 10.0\nillum 1\n\n"
        mtl_content += "newmtl mat_rims\nKa 0.2 0.2 0.2\nKd 0.75 0.75 0.75\nKs 0.9 0.9 0.9\nNs 90.0\nillum 2\n\n"
        mtl_content += "newmtl mat_headlights\nKa 0.3 0.3 0.3\nKd 1.0 1.0 1.0\nKs 1.0 1.0 1.0\nNs 120.0\nillum 2\n\n"
        mtl_content += "newmtl mat_taillights\nKa 0.2 0.02 0.02\nKd 0.9 0.1 0.1\nKs 0.8 0.3 0.3\nNs 60.0\nillum 2\n\n"
    else:
        mtl_content += "newmtl mat_walls\nKa 0.15 0.15 0.15\nKd 0.85 0.82 0.78\nKs 0.2 0.2 0.2\nNs 20.0\nillum 1\n\n"
        mtl_content += "newmtl mat_roof\nKa 0.15 0.05 0.05\nKd 0.75 0.25 0.18\nKs 0.3 0.3 0.3\nNs 30.0\nillum 1\n\n"
        mtl_content += "newmtl mat_windows\nKa 0.1 0.2 0.3\nKd 0.35 0.65 0.9\nKs 0.9 0.9 0.9\nNs 90.0\nillum 2\n\n"

    # Write OBJ content
    obj_content = f"# Model: {out_base_name}\nmtllib {out_base_name}.mtl\n\n"
    for v in verts:
        obj_content += f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n"
    for n in final_normals:
        obj_content += f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n"

    for mat_name, f_list in mat_faces.items():
        if not f_list: continue
        obj_content += f"\nusemtl {mat_name}\n"
        for f in f_list:
            i0, i1, i2 = f[0] + 1, f[1] + 1, f[2] + 1
            obj_content += f"f {i0}//{i0} {i1}//{i1} {i2}//{i2}\n"

    for out_dir in TARGET_DIRS:
        os.makedirs(out_dir, exist_ok=True)
        with open(os.path.join(out_dir, f"{out_base_name}.obj"), 'w') as fp:
            fp.write(obj_content)
        with open(os.path.join(out_dir, f"{out_base_name}.mtl"), 'w') as fp:
            fp.write(mtl_content)

    print(f"Done {out_base_name}: {len(verts)} verts, {len(faces)} tris, center=({cx:.2f}, {min_y:.2f}, {cz:.2f})")

def main():
    # 1. Player car from car.obj / car.mtl (Exact authentic 3DMA paint: Charcoal body #484848, Blue roof #3E49AB, White lights #FFFFFF, Red rear #DF250B)
    with open('/root/game-balap-3d/assets/car.obj', 'r') as f:
        car_obj = f.read().replace('mtllib car.mtl', 'mtllib player_car.mtl')
    with open('/root/game-balap-3d/assets/car.mtl', 'r') as f:
        car_mtl = f.read().replace('# Material file for car', '# Material file for player_car')

    for d in TARGET_DIRS:
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, 'player_car.obj'), 'w') as fp: fp.write(car_obj)
        with open(os.path.join(d, 'player_car.mtl'), 'w') as fp: fp.write(car_mtl)
    print("Done player_car (from authentic 3DMA model & colors)")

    # 2. Bot car 1 (from mobil2.obj, Racing Red)
    process_model('mobil2.obj', 'bot_car', is_car=True, body_rgb=(0.88, 0.12, 0.10))

    # 3. Bot car 2 (from mobil3.obj, Cyber Gold)
    process_model('mobil3.obj', 'bot_car2', is_car=True, body_rgb=(0.95, 0.72, 0.05))

    # 4. Building Villa (from rumah.obj)
    process_model('rumah.obj', 'building_villa', is_car=False)

    # 5. Building Apartment (from rumah(1).obj)
    process_model('rumah(1).obj', 'building_apt', is_car=False)

if __name__ == "__main__":
    main()
