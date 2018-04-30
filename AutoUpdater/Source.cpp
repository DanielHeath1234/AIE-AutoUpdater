#include "AutoUpdater.h"

int main()
{
	// TODO: Directory for where the updater need to download and install the update. Can I locate where to do this without user/dev input?
	// TODO: Will need to close the program to overwrite current data. Should updater be threaded in some way?

	auto Updater = new AutoUpdater(Version("1.0"), // Current Project Version.
		"https://raw.githubusercontent.com/DanielHeath1234/AIE-AutoUpdater/master/version", // Download link to raw text file containing a version number.
		"https://github.com/DanielHeath1234/AIE-AutoUpdater/archive/master.zip"); // Download link to .zip of newest version.

	// Updater error checks.
	if (Updater->error() != UPDATER_SUCCESS)
		return Updater->error();

	delete Updater;
	return 0;
}