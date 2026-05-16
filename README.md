# NTVDBM

<img src:https://raw.githubusercontent.com/RicardoRamosWorks/ricardoramosworks.github.io/refs/heads/main/Images/ntVDBM.png></img>
NT Virtual DosBox Machine This is a project that aims to transparently replace NTDVM with DOSBox. Developed for use in XP installations with UEFI, as a solution for playing Doom.

My handler  runs MS-DOS applications without the need for NTVDM (since the Apple TV doesn't support BIOS). Furthermore, it creates a configuration file per folder (the configuration files are located in system32, but it creates a shortcut in the folder for them). The configuration files are created using the folder name + crc16 hash to ensure it's not the same game. The configuration file is global for the game folder (for example, c:\games\doom); all other executables (like setup.exe) will also use the same configuration file.

There's also my "shortcut creator" system. Unfortunately, due to limitations in how Windows handles DOS applications, it's impossible to create a shortcut for a 16-bit executable without it becoming a PIF (Processed Intent File). PIFs depend on NTVDM, so this forced me to write a completely new solution from scratch, which was great because I was able to make improvements. The global context menu now has an item that creates a shortcut to the game on the desktop. It actually creates a .bat file with the game's name. This .bat file handles starting the game correctly, then it creates a shortcut to this .bat file on the desktop. But not only that, it will also automatically set the icon if the corresponding icon exists in the icons folder. 

To better understand: There's an icons folder in system32 called gameicons, inside it there's default.ico and icons for some games I've already created. If you create a shortcut for doom.exe, this handler will copy default.ico to doom+hash.ico. The hash is calculated based on the executable file, so icons can be shared among users to form a large icon library. I'm creating icons for the games I own and use, and I'll make them available together. If an icon already exists in the icon folder with the game's name and hash, the shortcut is created with the correct icon!

When you click on the game, DOSBox is called with all the arguments and path to run the game correctly. When the game finishes, it automatically receives the exit command and returns to the desktop.

It has a refined executable detection system to correctly differentiate between 32-bit and 16-bit (Windows 3.1) executables and 16-bit MS-DOS executables.

Currently, all MS-DOS executables and combos work wonderfully. But 16-bit Windows executables don't work; I'm working on a "winbox.exe" to work around this.

The project has a selection of pre-configured icons for some classic games I own. It's not guaranteed that your doom.exe will appear with the correct icon, as the icon is associated with the executable via checksum. So, if your doom.exe is a different version than the one I used, you will have to create the icon for your executable.

<b>USAGE:</b>

You need to download dosbox yourself, and place the executable and configuration files in system32, such as "dosbox.exe" and "dosbox.conf". Then, copy the files from this project (the gameicons folder, config, hdl.exe, and sendtotool.exe) to system32 as well. Merge the install.reg registry file (just double-click it), and everything will be ready.

There are two other registry files, enable handler and disable handler; they act as the on/off switch for everything. Some specific programs may not work well with my project, so you may need to disable them before running certain software (such as DrivePack Solution).

<b>Creating Icons</b>

"Create" wouldn't be the ideal word, but I couldn't find another. You don't necessarily need to create an icon from scratch. When you create a shortcut, the handler will copy default16.ico or default32.ico to a new file; the file name will be the same as the executable+hash. If a doom+hash.ico file already exists in the folder, but your shortcut to doom didn't receive the correct icon, your executable version is different and generated a different hash. To solve this, simply find the generic icon for doom+hash.ico, copy the icon with the image you want, and rename it accordingly.

<b>Compatibility</b>

All the games I tested from 1987 to 1999 ran perfectly (in the sense that there were no problems with the handler), I believe I achieved 100% compatibility with anything that should run in DOSBox.

Win32 executables are passed to the kernel normally; I believe I've fixed the problem with too many arguments in the call, so the handler (so far) no longer needs to be disabled for compatibility. The Shortcut Creator also differentiates between MSDOS icons and Windows icons when creating the generic icon.

<b>The future</b>

I intend to scale this project to something much larger, creating a universal game launcher using only Windows shell resources – something Windows should have had since Windows 98, in my opinion – a CLSID folder (a system folder, like My Documents or My Computer) that stores game shortcuts, but not that awful thing that Windows Vista and 7 had. I'd like something that creates shortcuts within it, and that uses a database containing information about the game, such as year, developer, publisher, screenshot, cover art, etc., all directly in the Explorer shell, and built in a simple way, with fast and transparent execution.

I've had this idea for many years. I tried using Launchbox; in my opinion, it's excellent, but very bloated and extremely heavy for a simple, old machine. I'm the guy who hates hardware obsolescence. I believe it's VERY important to find ways to reuse old hardware. Humanity is heading towards stupidity with the excess of interpreted languages; the web has become something ridiculous that requires 8GB of RAM to access a Wikipedia page. Old games are the main and most obvious way to make this hardware useful and durable.