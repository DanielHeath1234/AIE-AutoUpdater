#include "AutoUpdater.h"

int main()
{
	// TODO: Make into a dll. https://msdn.microsoft.com/en-us/library/ms235636.aspx <-ezlyfe
	// TODO: Create note of what input is expected for _SetDirs() to function correctly. 
	// TODO: Unzip update directly into install folder to save extra folder manipulation ?

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