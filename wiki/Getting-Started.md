# Getting Started with TMOD

This guide walks you through compiling TMOD from source, launching a local development server, connecting with the TMOD client, and utilizing the testing sandbox.

---

## 1. System Requirements

- **Operating System:** Windows 10 or Windows 11 (64-bit)
- **IDE:** Visual Studio 2022 (Community, Professional, or Enterprise)
- **Workloads Required in Visual Studio Installer:**
  - *Desktop development with C++*
  - *C++ MFC for latest build tools (x86 & x64)*
- **DirectX SDK:** Microsoft DirectX SDK (June 2010)
- **GTA: San Andreas:** A clean, unmodded GTA:SA v1.0 US installation directory

---

## 2. Compilation Steps

1. Open PowerShell or Command Prompt in your repository root:
   ```cmd
   cd c:\Users\dogu\Desktop\mtasa-blue
   ```

2. Generate Visual Studio project files using Premake:
   ```cmd
   .\win-create-projects.bat
   ```

3. Open `Build/MTASA.sln` in Visual Studio 2022.

4. Build the client binaries:
   - Select **Release** configuration.
   - Select **Win32** platform.
   - Build `Client Launcher` and `Client Deathmatch` (or Build Solution).
   - Binaries output to `Bin/TMOD.exe` and `Bin/mods/deathmatch/client.dll`.

5. Build the server binaries:
   - Select **Release** configuration.
   - Select **x64** platform.
   - Build `Server Launcher` and `Server Deathmatch`.
   - Binaries output to `Bin/server/TMODServer64.exe` and `Bin/server/x64/deathmatch.dll`.

---

## 3. Running a Local Server

1. Navigate to `Bin/server/`.
2. Launch `TMODServer64.exe` (or use the launcher).
3. The server starts listening on UDP port `22003`.
4. In the server console, verify that `tmod_tester` is started:
   ```
   start tmod_tester
   ```

---

## 4. Connecting and Testing

1. Run `Bin/TMOD.exe`.
2. Connect to your local server (`connect localhost:22003` or use the Server Browser).
3. Once in-game:
   - Press **`F5`** to open the **TMOD Developer & Testing Control Panel**.
   - Use the **Hareket (Movement)** tab to toggle between Source locomotion and GTA native physics, test air-acceleration presets, and enable auto-bhop.
   - Use the **Kamera (Camera)** tab to switch between First Person, Source Third Person, and GTA cameras.
   - Use the **Fizik (Physics)** tab to spawn Bullet dynamic rigid bodies, apply forces, and test collision against the world.
   - Use `/tmod` in the chat to display the debug HUD with real-time speed, movement modes, and camera angles.
