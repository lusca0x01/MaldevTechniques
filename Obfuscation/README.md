## Obfuscation

### Explanation

This script demonstrates how PowerShell can execute base64-encoded commands, even when non-printable characters, such as null (0x00) bytes, are inserted between the encoded characters. In this particular example, the script executes a harmless Rick Roll, but the primary goal is to show that PowerShell can decode and run obfuscated scripts, even when non-printable characters like null bytes are included.

### Steps to Run the Script

1. **Open PowerShell as Administrator:**
   - Press `Windows + X` and choose **Windows PowerShell (Admin)**, or search for `powershell` in the start menu, right-click it, and select **Run as Administrator**.

2. **Set Execution Policy:**
   To allow the script to run, you will need to change the execution policy. Run the following command in your elevated PowerShell window:

   ```powershell
   Set-ExecutionPolicy RemoteSigned -Scope CurrentUser -Force
   ```

3. **Run the script**
  ```powershell
  .\obfuscation.ps1
  ```

Feel free to expand on this example. For instance, you could create a task scheduler that starts a multi-stage attack. This could involve chaining several encoded scripts or actions to simulate more complex attack scenarios.