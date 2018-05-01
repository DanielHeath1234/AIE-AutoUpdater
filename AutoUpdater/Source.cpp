#include "AutoUpdater.h"
#include <thread>
#include <chrono>

int AutoUpdaterThread()
{
	auto Updater = new AutoUpdater(Version("1.0"), // Current Project Version.
		"https://raw.githubusercontent.com/DanielHeath1234/AIE-AutoUpdater/master/version", // Download link to raw text file containing a version number.
		"https://github.com/DanielHeath1234/AIE-AutoUpdater/archive/master.zip"); // Download link to .zip of newest version.

	 // Updater error checks.
	if (Updater->error() != UPDATER_SUCCESS)
		return Updater->error(); 

	delete Updater;
	return UPDATER_SUCCESS;
}

int main()
{
	// TODO: Make into a dll. https://msdn.microsoft.com/en-us/library/ms235636.aspx <-ezlyfe

	// TODO: Directory for where the updater will need to download and install the update. Can I locate where to do this without user/dev input? 
	// "WorkingDir\..\" to be the place to overwrite?

	// TODO: Will need to close the program to overwrite current data. Should updater be threaded in some way?
	// Add this to a new thread, make daemon thred and when finished updating or upon error return result and delete thread.

	// Don't detach thread until unzip?
	// Use future to get value from download and if it matches a UZ_READY return then close process and detach?
	// Updater downloads and unzips. Once that is successful, detach thread to overwrite files and once complete, launch installed update.

	// Updater downloads and unzips. Once that is successful launches other program to install files and then launch installed update.
	// How does other program know what files to install and where? As well as relaunching.

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

																				  // Updater error checks.
	if (Updater->error() != UPDATER_SUCCESS)
		return Updater->error();

	delete Updater;

	//std::thread updater(AutoUpdaterThread);
	//updater.join();
	//std::this_thread::sleep_for(std::chrono::seconds(20));

	return 0;
}