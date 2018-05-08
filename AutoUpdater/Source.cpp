#include "AutoUpdater.h"

int main()
{
	// TODO: Make into a dll. https://msdn.microsoft.com/en-us/library/ms235636.aspx <-ezlyfe

	// TODO: Overwrite folder that current process is running in.

	// TODO: Fix constructor to run the process_location through the _SetDirs() function. 
	// Have note of what input is expected for it to function properly.

	/*
	Now for a cool trick.How do we solve the chicken and egg problem ? 
	One of the payloads might contain a replacement for your application's executable itself, but it can't be overwritten while it is running.
	This is enforced by the operating system. However, Windows does(reason unknown) allow a running executable to be renamed!
	First we rename the application to [application].exe.bak and then we copy that file back to [application].exe.
	The file lock has been moved to the backup file, so if a payload contains a replacement, it will overwrite [application].exe.
	If it is not being replaced, no harm done.

	To update the application we copy everything in the work directory to the application directory, and then delete the work directory.
	*/

	//http://en.cppreference.com/w/cpp/filesystem

	auto Updater = new AutoUpdater(Version("1.0"), // Current Project Version.
		"https://raw.githubusercontent.com/DanielHeath1234/AIE-AutoUpdater/master/version", // Download link to raw text file containing a version number.
		"https://github.com/DanielHeath1234/AIE-AutoUpdater/archive/master.zip"); // Download link to .zip of newest version. 
																				  // Path to process that runs the app (including extension).
	// Updater error checks.
	if (Updater->error() != UPDATER_SUCCESS)
		return Updater->error();
		
	delete Updater;

	return 0;
}