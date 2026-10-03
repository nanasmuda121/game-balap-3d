#!/usr/bin/env python3
import json
import os

SOURCE_3MA = "/storage/emulated/0/Download/src/mobil.3ma"
OUTPUT_DIR = "/root/game-balap-3d/assets"

def hex_to_rgb(hex_str):
    hex_str = hex_str.strip().lstrip('#')
    if len(hex_str) == 6:
        r = int(hex_str[0:2], 16) / 255.0
        g = int(hex_str[2:4], 16) / 255.0
        b = int(hex_str[4:6], 16) / 255.0
        return (r, g, b)
    return (0.5, 0.5, 0.5)

def convert():
    if not os.path.exists(SOURCE_3MA):
        print(f"Error: {SOURCE_3MA} not found!")
        return

    with open(SOURCE_3MA, 'r') as f:
        data = json.load(f)

    # Mesh 1 is the 3D car model
    mesh = data['meshes'][1]
    factor = float(mesh.get('preciseFactor', 10000))
    raw_pos = mesh['_positions']
    indices = mesh['_posIndexis']
    normals = mesh.get('_normals', [])
    colors = mesh.get('_colorsHtml', [])
    tri_count = len(indices) // 3

    # Calculate center for perfect alignment
    xs = [raw_pos[j]/factor for j in range(0, len(raw_pos), 3)]
    ys = [raw_pos[j+1]/factor for j in range(0, len(raw_pos), 3)]
    zs = [raw_pos[j+2]/factor for j in range(0, len(raw_pos), 3)]

    cx = (min(xs) + max(xs)) / 2.0
    min_y = min(ys)
    cz = (min(zs) + max(zs)) / 2.0

    print(f"Centering car model: Offset X={cx:.4f}, Y={min_y:.4f}, Z={cz:.4f}")

    # Build unique colors
    unique_colors = sorted(list(set(colors)))
    print(f"Unique colors found: {unique_colors}")

    def export_variant(filename_base, color_override=None):
        obj_path = os.path.join(OUTPUT_DIR, f"{filename_base}.obj")
        mtl_path = os.path.join(OUTPUT_DIR, f"{filename_base}.mtl")

        # Write MTL
        with open(mtl_path, 'w') as mtl_f:
            mtl_f.write(f"# Material file for {filename_base}\n\n")
            for col in unique_colors:
                target_col = col
                if color_override and col in color_override:
                    target_col = color_override[col]
                r, g, b = hex_to_rgb(target_col)
                mtl_f.write(f"newmtl mat_{col}\n")
                mtl_f.write(f"Ka {r*0.2:.4f} {g*0.2:.4f} {b*0.2:.4f}\n")
                mtl_f.write(f"Kd {r:.4f} {g:.4f} {b:.4f}\n")
                mtl_f.write(f"Ks 0.5 0.5 0.5\n")
                mtl_f.write(f"Ns 50.0\n")
                mtl_f.write(f"d 1.0\n")
                mtl_f.write(f"illum 2\n\n")

        # Group triangles by material
        tri_by_mat = {}
        for t in range(tri_count):
            col = colors[t*3]
            if col not in tri_by_mat:
                tri_by_mat[col] = []
            tri_by_mat[col].append(t)

        # Write OBJ
        with open(obj_path, 'w') as obj_f:
            obj_f.write(f"# 3D Car Model converted from mobil.3ma\n")
            obj_f.write(f"mtllib {filename_base}.mtl\n\n")

            # Write all vertices (centered, normalized)
            # In mobil.3ma, Z is length, Y is height, X is width.
            # Front is -Z, Back is +Z.
            for i in range(len(raw_pos) // 3):
                x = (raw_pos[i*3] / factor) - cx
                y = (raw_pos[i*3 + 1] / factor) - min_y
                z = (raw_pos[i*3 + 2] / factor) - cz
                obj_f.write(f"v {x:.4f} {y:.4f} {z:.4f}\n")

            # Write normals if available
            has_normals = len(normals) >= len(indices) * 3
            if has_normals:
                for i in range(len(indices)):
                    nx = normals[i*3] / factor
                    ny = normals[i*3 + 1] / factor
                    nz = normals[i*3 + 2] / factor
                    obj_f.write(f"vn {nx:.4f} {ny:.4f} {nz:.4f}\n")

            # Write faces grouped by material
            for col, tris in tri_by_mat.items():
                obj_f.write(f"\nusemtl mat_{col}\n")
                for t in tris:
                    i0 = indices[t*3] + 1
                    i1 = indices[t*3 + 1] + 1
                    i2 = indices[t*3 + 2] + 1
                    if has_normals:
                        n0 = (t*3) + 1
                        n1 = (t*3 + 1) + 1
                        n2 = (t*3 + 2) + 1
                        obj_f.write(f"f {i0}//{n0} {i1}//{n1} {i2}//{n2}\n")
                    else:
                        obj_f.write(f"f {i0} {i1} {i2}\n")

        print(f"Generated: {obj_path} & {mtl_path}")

    # 1. Base Car Model
    export_variant("car")

    # 2. Player Car (Sporty Electric Blue body)
    export_variant("player_car", {
        '484848': '2255CC', # Main body to vibrant sports blue
        '3E49AB': '002288'  # Roof accent to dark navy
    })

    # 3. Bot Car (Racing Crimson Red body)
    export_variant("bot_car", {
        '484848': 'CC2211', # Main body to racing crimson
        '3E49AB': '771100'  # Roof accent to deep maroon
    })

if __name__ == "__main__":
    convert()
