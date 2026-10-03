#!/usr/bin/env python3
import os
import math
import struct
import json

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

def export_glb(out_base_name, verts, normals, mat_faces, mat_colors):
    # mat_faces: { mat_name: [ (v0, v1, v2), ... ] }
    # mat_colors: { mat_name: (r, g, b, a) }
    bin_data = bytearray()
    def append_data(data_bytes):
        offset = len(bin_data)
        bin_data.extend(data_bytes)
        while len(bin_data) % 4 != 0: bin_data.append(0)
        return offset, len(data_bytes)

    materials_list = []
    primitives_list = []
    buffer_views = []
    accessors = []

    mat_name_to_id = {}
    for mat_name, col in mat_colors.items():
        mat_id = len(materials_list)
        mat_name_to_id[mat_name] = mat_id
        materials_list.append({
            'name': mat_name,
            'pbrMetallicRoughness': {
                'baseColorFactor': [col[0], col[1], col[2], col[3] if len(col) > 3 else 1.0],
                'metallicFactor': 0.1,
                'roughnessFactor': 0.6
            }
        })

    for mat_name, faces in mat_faces.items():
        if not faces:
            continue
        mat_id = mat_name_to_id.get(mat_name, 0)

        # Unique vertices used in this material primitive
        used_v_indices = sorted(list(set(vi for f in faces for vi in f)))
        v_remap = {orig: new for new, orig in enumerate(used_v_indices)}

        prim_indices = []
        for f in faces:
            prim_indices.extend([v_remap[f[0]], v_remap[f[1]], v_remap[f[2]]])

        prim_positions = []
        prim_normals = []
        prim_uvs = []
        for vi in used_v_indices:
            prim_positions.extend(verts[vi])
            prim_normals.extend(normals[vi])
            # Generative simple cylindrical/planar UVs
            prim_uvs.extend([verts[vi][0] * 0.5 + 0.5, verts[vi][1] * 0.5 + 0.5])

        count_v = len(used_v_indices)
        count_i = len(prim_indices)

        # Append Index data
        idx_format = '<H' if count_v < 65535 else '<I'
        component_type = 5123 if count_v < 65535 else 5125
        i_bytes = struct.pack(f'<{count_i}' + ('H' if count_v < 65535 else 'I'), *prim_indices)
        i_off, i_len = append_data(i_bytes)

        # Append Position data
        p_bytes = struct.pack(f'<{count_v*3}f', *prim_positions)
        p_off, p_len = append_data(p_bytes)

        # Append Normal data
        n_bytes = struct.pack(f'<{count_v*3}f', *prim_normals)
        n_off, n_len = append_data(n_bytes)

        # Append UV data
        u_bytes = struct.pack(f'<{count_v*2}f', *prim_uvs)
        u_off, u_len = append_data(u_bytes)

        # Calculate bounding box for POSITIONS
        xs = [prim_positions[k*3] for k in range(count_v)]
        ys = [prim_positions[k*3+1] for k in range(count_v)]
        zs = [prim_positions[k*3+2] for k in range(count_v)]
        min_p = [min(xs), min(ys), min(zs)]
        max_p = [max(xs), max(ys), max(zs)]

        # Buffer views
        bv_i = len(buffer_views)
        buffer_views.append({'buffer': 0, 'byteOffset': i_off, 'byteLength': i_len, 'target': 34963})
        bv_p = len(buffer_views)
        buffer_views.append({'buffer': 0, 'byteOffset': p_off, 'byteLength': p_len, 'target': 34962})
        bv_n = len(buffer_views)
        buffer_views.append({'buffer': 0, 'byteOffset': n_off, 'byteLength': n_len, 'target': 34962})
        bv_u = len(buffer_views)
        buffer_views.append({'buffer': 0, 'byteOffset': u_off, 'byteLength': u_len, 'target': 34962})

        # Accessors
        acc_i = len(accessors)
        accessors.append({'bufferView': bv_i, 'byteOffset': 0, 'componentType': component_type, 'count': count_i, 'type': 'SCALAR'})
        acc_p = len(accessors)
        accessors.append({'bufferView': bv_p, 'byteOffset': 0, 'componentType': 5126, 'count': count_v, 'type': 'VEC3', 'min': min_p, 'max': max_p})
        acc_n = len(accessors)
        accessors.append({'bufferView': bv_n, 'byteOffset': 0, 'componentType': 5126, 'count': count_v, 'type': 'VEC3'})
        acc_u = len(accessors)
        accessors.append({'bufferView': bv_u, 'byteOffset': 0, 'componentType': 5126, 'count': count_v, 'type': 'VEC2'})

        primitives_list.append({
            'attributes': {
                'POSITION': acc_p,
                'NORMAL': acc_n,
                'TEXCOORD_0': acc_u
            },
            'indices': acc_i,
            'material': mat_id,
            'mode': 4
        })

    gltf = {
        'asset': {'version': '2.0', 'generator': 'RaceDriveExporter'},
        'scenes': [{'nodes': [0]}],
        'nodes': [{'mesh': 0}],
        'materials': materials_list,
        'meshes': [{'primitives': primitives_list}],
        'buffers': [{'byteLength': len(bin_data)}],
        'bufferViews': buffer_views,
        'accessors': accessors
    }

    json_str = json.dumps(gltf, separators=(',', ':')).encode('utf-8')
    while len(json_str) % 4 != 0: json_str += b' '

    total_len = 12 + 8 + len(json_str) + 8 + len(bin_data)
    glb = bytearray()
    glb.extend(struct.pack('<III', 0x46546C67, 2, total_len))
    glb.extend(struct.pack('<II', len(json_str), 0x4E4F534A))
    glb.extend(json_str)
    glb.extend(struct.pack('<II', len(bin_data), 0x004E4942))
    glb.extend(bin_data)

    for out_dir in TARGET_DIRS:
        os.makedirs(out_dir, exist_ok=True)
        with open(os.path.join(out_dir, f"{out_base_name}.glb"), 'wb') as fp:
            fp.write(glb)

    print(f"Exported GLB: {out_base_name}.glb ({len(primitives_list)} primitives, {len(glb)} bytes)")

def export_obj(out_base_name, verts, normals, mat_faces, mat_colors):
    # Write MTL
    mtl_content = f"# Material file for {out_base_name}\n\n"
    for mat_name, col in mat_colors.items():
        r, g, b = col[0], col[1], col[2]
        mtl_content += f"newmtl {mat_name}\n"
        mtl_content += f"Ka {r*0.2:.3f} {g*0.2:.3f} {b*0.2:.3f}\n"
        mtl_content += f"Kd {r:.3f} {g:.3f} {b:.3f}\n"
        mtl_content += f"Ks 0.5 0.5 0.5\n"
        mtl_content += f"Ns 50.0\n"
        mtl_content += f"d 1.0\n"
        mtl_content += f"illum 2\n\n"

    # Write OBJ with explicit vt to prevent Raylib LoadOBJ crash!
    obj_content = f"# 3D Model: {out_base_name}\nmtllib {out_base_name}.mtl\n\n"
    for v in verts:
        obj_content += f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n"
    for n in normals:
        obj_content += f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n"
    obj_content += "vt 0.0000 0.0000\n" # GUARANTEES vt_idx is 0 and valid in Raylib LoadOBJ!

    for mat_name, f_list in mat_faces.items():
        if not f_list: continue
        obj_content += f"\nusemtl {mat_name}\n"
        for f in f_list:
            i0, i1, i2 = f[0] + 1, f[1] + 1, f[2] + 1
            # Face format: v/vt/vn -> v/1/vn
            obj_content += f"f {i0}/1/{i0} {i1}/1/{i1} {i2}/1/{i2}\n"

    for out_dir in TARGET_DIRS:
        os.makedirs(out_dir, exist_ok=True)
        with open(os.path.join(out_dir, f"{out_base_name}.obj"), 'w') as fp:
            fp.write(obj_content)
        with open(os.path.join(out_dir, f"{out_base_name}.mtl"), 'w') as fp:
            fp.write(mtl_content)

    print(f"Exported OBJ: {out_base_name}.obj & {out_base_name}.mtl ({len(verts)} verts)")

def process_car_obj_player():
    # Authentic 3DMA car
    src_obj = "/root/game-balap-3d/assets/car.obj"
    with open(src_obj, 'r') as f:
        lines = f.readlines()

    verts = []
    normals = []
    mat_faces = {}
    current_mat = 'mat_484848'

    for line in lines:
        if line.startswith('v '):
            p = line.split()
            verts.append((float(p[1]), float(p[2]), float(p[3])))
        elif line.startswith('vn '):
            p = line.split()
            normals.append((float(p[1]), float(p[2]), float(p[3])))
        elif line.startswith('usemtl '):
            current_mat = line.split()[1]
            if current_mat not in mat_faces:
                mat_faces[current_mat] = []
        elif line.startswith('f '):
            parts = line.strip().split()[1:]
            idx = [int(p.split('/')[0]) - 1 for p in parts]
            if current_mat not in mat_faces:
                mat_faces[current_mat] = []
            mat_faces[current_mat].append((idx[0], idx[1], idx[2]))

    # Ensure normals count matches vertices count
    if len(normals) != len(verts):
        vert_normals = [[0.0, 0.0, 0.0] for _ in range(len(verts))]
        for mat, faces in mat_faces.items():
            for f in faces:
                fn = calc_normal(verts[f[0]], verts[f[1]], verts[f[2]])
                for vi in f:
                    vert_normals[vi][0] += fn[0]
                    vert_normals[vi][1] += fn[1]
                    vert_normals[vi][2] += fn[2]
        normals = []
        for vn in vert_normals:
            l = math.sqrt(vn[0]*vn[0] + vn[1]*vn[1] + vn[2]*vn[2])
            normals.append((vn[0]/l, vn[1]/l, vn[2]/l) if l > 1e-6 else (0.0, 1.0, 0.0))

    mat_colors = {
        'mat_484848': (0.282, 0.282, 0.282, 1.0), # Authentic Dark Charcoal Body
        'mat_3E49AB': (0.243, 0.286, 0.671, 1.0), # Authentic Blue Roof
        'mat_FFFFFF': (1.000, 1.000, 1.000, 1.0), # Authentic White Headlights
        'mat_DF250B': (0.875, 0.145, 0.043, 1.0), # Authentic Red Taillights
        'mat_A0A0A0': (0.627, 0.627, 0.627, 1.0), # Authentic Grey Wheels
        'mat_D3D3D3': (0.827, 0.827, 0.827, 1.0)  # Authentic Silver Trim
    }

    export_glb("player_car", verts, normals, mat_faces, mat_colors)
    export_obj("player_car", verts, normals, mat_faces, mat_colors)

def process_raw_obj(in_filename, out_base_name, is_car=True, body_rgb=(0.88, 0.12, 0.10)):
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
            # Skip dummy cube in name0
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

    normals = []
    for vn in vert_normals:
        l = math.sqrt(vn[0]*vn[0] + vn[1]*vn[1] + vn[2]*vn[2])
        normals.append((vn[0]/l, vn[1]/l, vn[2]/l) if l > 1e-6 else (0.0, 1.0, 0.0))

    mat_faces = {}
    mat_colors = {}

    if is_car:
        mat_faces['mat_body'] = []
        mat_faces['mat_glass'] = []
        mat_faces['mat_tires'] = []
        mat_faces['mat_rims'] = []
        mat_faces['mat_headlights'] = []
        mat_faces['mat_taillights'] = []

        mat_colors['mat_body'] = (body_rgb[0], body_rgb[1], body_rgb[2], 1.0)
        mat_colors['mat_glass'] = (0.15, 0.25, 0.35, 1.0)
        mat_colors['mat_tires'] = (0.12, 0.12, 0.12, 1.0)
        mat_colors['mat_rims'] = (0.75, 0.75, 0.75, 1.0)
        mat_colors['mat_headlights'] = (1.00, 1.00, 1.00, 1.0)
        mat_colors['mat_taillights'] = (0.90, 0.10, 0.10, 1.0)

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

        if 'villa' in out_base_name:
            mat_colors['mat_walls'] = (0.85, 0.82, 0.78, 1.0)
            mat_colors['mat_roof'] = (0.75, 0.25, 0.18, 1.0)
            mat_colors['mat_windows'] = (0.35, 0.65, 0.90, 1.0)
        else:
            mat_colors['mat_walls'] = (0.55, 0.60, 0.65, 1.0)
            mat_colors['mat_roof'] = (0.22, 0.28, 0.35, 1.0)
            mat_colors['mat_windows'] = (0.30, 0.55, 0.80, 1.0)

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

    export_glb(out_base_name, verts, normals, mat_faces, mat_colors)
    export_obj(out_base_name, verts, normals, mat_faces, mat_colors)

def main():
    print("=== BUILDING ALL OPTIMIZED 3D ASSETS (GLB & OBJ) ===")
    # 1. Player Car (Authentic 3DMA paint)
    process_car_obj_player()

    # 2. Bot Car 1 (Racing Red)
    process_raw_obj('mobil2.obj', 'bot_car', is_car=True, body_rgb=(0.88, 0.12, 0.10))

    # 3. Bot Car 2 (Cyber Gold)
    process_raw_obj('mobil3.obj', 'bot_car2', is_car=True, body_rgb=(0.95, 0.72, 0.05))

    # 4. Building Villa
    process_raw_obj('rumah.obj', 'building_villa', is_car=False)

    # 5. Building Apartment
    process_raw_obj('rumah(1).obj', 'building_apt', is_car=False)
    print("=== ALL ASSETS GENERATED SUCCESSFULLY ===")

if __name__ == "__main__":
    main()
