# 🏎️ RaceDrive — Player vs Bot (C++ Android Native APK)

Game balapan mobil 3D **100% C++ Native** (menggunakan OpenGL ES & Raylib).
- **Nama Game**: `RaceDrive`
- **Package Name**: `com.racedrive.nanas`
- **Model Mobil**: `/storage/emulated/0/Download/src/mobil.3ma`
- **Logo/Icon**: `/storage/emulated/0/Download/racedrive.png`

---

## 🌟 Fitur Utama Game

1. **3D Car Physics & Mechanics**:
   - Fisika akselerasi realistis, pengereman, transmisi mundur, dan drifting.
   - Efek partikel asap ban saat menikung tajam (*tire smoke*).
   - Sistem **Nitro Boost** dengan semburan api knalpot (*flame exhaust*) dan efek pelebaran sudut pandang kamera (*dynamic FOV*).
   - Deteksi tabrakan fisik (*elastic collision*) antara mobil pemain dan bot, serta pantulan dinding sirkuit.

2. **Sirkuit & Arena 3D Lengkap (Grand Prix Circuit ~784 Meter)**:
   - **Lintasan Aspal**: Jalur balap tertutup dengan garis marka putih putus-putus dan garis batas jalan.
   - **Kerbs / Curbs**: Pembatas tikungan belang merah-putih di setiap sudut apex tikungan.
   - **Armco Guardrails**: Pagar pengaman baja 3D di sisi kiri dan kanan sepanjang sirkuit agar mobil tidak terlempar keluar.
   - **Garis Start / Finish**: Banner bendera kotak-kotak (*checkered flag*) dan gerbang lengkung 3D (*Truss Arch*).
   - **Dekorasi Arena**: Tribun penonton (*Grandstand*), papan sponsor (*Billboards*: TURBO, NITRO, CHAMPIONSHIP), dan pepohonan 3D di infield/outfield.

3. **Lawan AI Pintar (Bot Racer)**:
   - AI navigasi mengikuti racing line sirkuit menggunakan algoritma *waypoint lookahead*.
   - Mampu memperlambat laju sebelum tikungan tajam dan tancap gas serta menggunakan nitro di lintasan lurus.
   - Logika manuver menyalip (*tactical overtaking*) saat berada di belakang pemain.

4. **HUD & Kontrol Touchscreen HP Android**:
   - **Speedometer**: Penunjuk kecepatan digital KM/H dan bar gauge Nitro.
   - **Status Balapan**: Peringkat posisi *real-time* (1st / 2nd), Lap Counter (3 Laps), Lap Timer, dan Best Lap Time.
   - **Mini-Map Radar**: Radar 2D di pojok kanan atas yang menampilkan posisi mobil Pemain (biru) dan Bot (merah).
   - **Tombol Sentuh Responsif (Android)**:
     - Tombol **<** dan **>** di kiri bawah untuk kemudi.
     - Pedal **GAS** (Hijau) & **BRAKE / REVERSE** (Merah) di kanan bawah.
     - Tombol **NITRO** (Biru Neon) untuk dorongan turbo instan.
     - Tombol **RESET [R]** di atas untuk kembali ke tengah sirkuit jika terbalik.
   - Juga mendukung keyboard PC (W/A/S/D atau Panah, Space untuk Nitro, R untuk Reset).

---

## 📁 Struktur Direktori Proyek

```
game-balap-3d/
├── .github/
│   └── workflows/
│       └── build-apk.yml     <-- CI/CD GitHub Actions: Otomatis compile jadi .apk
├── android/                  <-- Proyek NativeActivity Android (Gradle & NDK)
│   ├── app/
│   │   ├── src/main/
│   │   │   ├── AndroidManifest.xml
│   │   │   ├── cpp/CMakeLists.txt
│   │   │   └── assets/       <-- Model 3D mobil.3ma yang telah dikonversi
│   │   └── build.gradle
│   ├── build.gradle
│   ├── settings.gradle
│   └── gradlew
├── assets/
│   ├── player_car.obj        <-- Model mobil pemain (Electric Blue livery)
│   ├── player_car.mtl
│   ├── bot_car.obj           <-- Model mobil musuh (Crimson Red livery)
│   ├── bot_car.mtl
│   ├── car.obj               <-- Model original
│   └── car.mtl
├── src/
│   ├── main.cpp              <-- Game loop & state management
│   ├── car.h / car.cpp       <-- Fisika mobil, partikel nitro/smoke, render 3D
│   ├── bot.h / bot.cpp       <-- AI navigator & taktik menyalip
│   ├── track.h / track.cpp   <-- Sirkuit 3D, curbs, dinding, radar mini-map
│   ├── camera_follow.h       <-- Third-person chase camera & camera shake
│   ├── ui.h / ui.cpp         <-- HUD speedometer, radar, touch controls
│   └── common.h              <-- Konstanta & vector math
├── CMakeLists.txt            <-- Build konfigurasi desktop/Linux
└── convert_model.py          <-- Script konversi mobil.3ma ke Wavefront OBJ
```

---

## 🚀 Cara Build Menjadi APK Lewat GitHub (Tanpa Install Apa pun di PC/HP)

Anda cukup melakukan push folder ini ke repository GitHub baru:

### Langkah 1: Buat Repository di GitHub
1. Buka [github.com/new](https://github.com/new) di browser Anda.
2. Beri nama repository, misalnya: `game-balap-3d`.
3. Klik **Create repository**.

### Langkah 2: Push Kode ke GitHub
Jalankan perintah berikut di terminal Anda:

```bash
cd /root/game-balap-3d
git init
git add .
git commit -m "Initial commit: 3D Racing Game Player vs Bot with mobil.3ma"
git branch -M main
git remote add origin https://github.com/USERNAME-ANDA/game-balap-3d.git
git push -u origin main
```
*(Ganti `USERNAME-ANDA` dengan username GitHub Anda)*

### Langkah 3: Download File APK
1. Buka repository Anda di GitHub, lalu klik tab **Actions**.
2. Anda akan melihat workflow **"Build Android APK (C++ 3D Game)"** sedang berjalan secara otomatis.
3. Tunggu sekitar 2 menit hingga muncul centang hijau ✅.
4. Klik pada hasil build tersebut, scroll ke bawah ke bagian **Artifacts**.
5. Klik **`Apex-3D-Racing-APK`** untuk mendownload file `.apk` langsung ke HP Anda.
6. Install di HP Android dan nikmati game balapan 3D Anda!
