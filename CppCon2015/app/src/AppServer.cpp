//
// AppServer.cpp
//
// The bundle container application
//
// SPDX-License-Identifier: Apache-2.0
//


#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/OSP/OSPSubsystem.h"
#include "Poco/OSP/ServiceRegistry.h"
#include "Poco/ErrorHandler.h"
#include "Poco/Environment.h"
#include "Poco/Format.h"
#include <iostream>


using Poco::Util::ServerApplication;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::OSP::OSPSubsystem;
using Poco::OSP::ServiceRegistry;


class AppServer: public ServerApplication
{
public:
	AppServer():
		_errorHandler(*this),
		_pOSP(new OSPSubsystem),
		_showHelp(false)
	{
		Poco::ErrorHandler::set(&_errorHandler);
		addSubsystem(_pOSP);
	}
	
	~AppServer()
	{
		// wait until all threads have terminated 
		// before we completely shut down.
		Poco::ThreadPool::defaultPool().joinAll();
	}
	
	ServiceRegistry& serviceRegistry()
	{
		return _pOSP->serviceRegistry();
	}

protected:
	class ErrorHandler: public Poco::ErrorHandler
	{
	public:
		ErrorHandler(AppServer& app):
			_app(app)
		{
		}
		
		void exception(const Poco::Exception& exc)
		{
			log(exc.displayText());
		}

		void exception(const std::exception& exc)
		{
			log(exc.what());
		}

		void exception()
		{
			log("unknown exception");
		}
		
		void log(const std::string& message)
		{
			_app.logger().error("A thread was terminated by an unhandled exception: " + message);
		}
		
	private:
		AppServer& _app;
	};

	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
		logger().information(Poco::format("System information: %s (%s) on %s, %u CPUs.",	
			Poco::Environment::osDisplayName(),
			Poco::Environment::osVersion(),
			Poco::Environment::osArchitecture(),
			Poco::Environment::processorCount()));
	}
	
	void defineOptions(OptionSet& options)
	{
		ServerApplication::defineOptions(options);

		options.addOption(
			Option("help", "h", "Display help information on command line arguments.")
				.required(false)
				.repeatable(false)
				.callback(OptionCallback<AppServer>(this, &AppServer::handleHelp)));

		options.addOption(
			Option("config-file", "c", "Load configuration data from a file.")
				.required(false)
				.repeatable(true)
				.argument("file")
				.callback(OptionCallback<AppServer>(this, &AppServer::handleConfig)));
	}
	
	void handleHelp(const std::string& name, const std::string& value)
	{
		_showHelp = true;
		displayHelp();
		stopOptionsProcessing();
		_pOSP->cancelInit();
	}

	void handleConfig(const std::string& name, const std::string& value)
	{
		loadConfiguration(value);
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("The macchina.io server.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args)
	{
		if (!_showHelp)
		{
			waitForTerminationRequest();
		}
		return Application::EXIT_OK;
	}

private:
	ErrorHandler _errorHandler;
	OSPSubsystem* _pOSP;
	bool _showHelp;
};


POCO_SERVER_MAIN(AppServer)
