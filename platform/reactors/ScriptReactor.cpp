// ------------------------------------------------------------------------
// Pion is a development platform for building Reactors that process Events
// ------------------------------------------------------------------------
// Copyright (C) 2007-2009 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Pion is free software: you can redistribute it and/or modify it under the
// terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// Pion is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
// more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with Pion.  If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <boost/scoped_array.hpp>
#include <pion/platform/CodecFactory.hpp>
#include "ScriptReactor.hpp"

using namespace pion::platform;


namespace pion {		// begin namespace pion
namespace plugins {		// begin namespace plugins


// static members of ScriptReactor

const std::string		ScriptReactor::INPUT_CODEC_ELEMENT_NAME = "InputCodec";
const std::string		ScriptReactor::OUTPUT_CODEC_ELEMENT_NAME = "OutputCodec";
const std::string		ScriptReactor::COMMAND_ELEMENT_NAME = "Command";


// ScriptReactor member functions

void ScriptReactor::start(void)
{
	ConfigWriteLock cfg_lock(*this);
	if (! m_is_running) {
		// open pipe to script
		openPipe();
		
		m_is_running = true;

		// spawn a new thread that will be used to read events from the script
		PION_LOG_DEBUG(m_logger, "Starting reader thread: " << getId());
		m_thread_ptr.reset(new boost::thread(boost::bind(&ScriptReactor::readEvents, this)));
	}
}
	
bool ScriptReactor::stopIfRunning(void)
{
	bool do_stop = false;
	{	
		ConfigWriteLock cfg_lock(*this);
		if (m_is_running) {
			PION_LOG_DEBUG(m_logger, "Waiting for reader thread to finish");
			do_stop = true;
			m_is_running = false;
			
			// this should break the reader thread out of any blocking operations
			CLOSE_DESCRIPTOR(m_output_pipe);
		}
	}

	if (do_stop) {
		// we have to release the write lock before joining, otherwise things deadlock =(
		m_thread_ptr->join();

		ConfigWriteLock cfg_lock(*this);
		m_thread_ptr.reset();
		closePipe();
	}
	
	return do_stop;
}

void ScriptReactor::setConfig(const Vocabulary& v, const xmlNodePtr config_ptr)
{
	// make sure it's not running (it needs to be restarted in case Command changes)
	const bool was_running = stopIfRunning();

	{
		// first set config options for the Reactor base class
		ConfigWriteLock cfg_lock(*this);
		Reactor::setConfig(v, config_ptr);
		
		// get the Input Codec that the Reactor should use
		if (! ConfigManager::getConfigOption(INPUT_CODEC_ELEMENT_NAME, m_input_codec_id, config_ptr))
			throw EmptyInputCodecException(getId());
		m_input_codec_ptr = getCodecFactory().getCodec(m_input_codec_id);
		PION_ASSERT(m_input_codec_ptr);
	
		// get the Output Codec that the Reactor should use
		if (! ConfigManager::getConfigOption(OUTPUT_CODEC_ELEMENT_NAME, m_output_codec_id, config_ptr))
			throw EmptyOutputCodecException(getId());
		m_output_codec_ptr = getCodecFactory().getCodec(m_output_codec_id);
		PION_ASSERT(m_output_codec_ptr);
		
		// get the script or program command to execute
		if (! ConfigManager::getConfigOption(COMMAND_ELEMENT_NAME, m_command, config_ptr))
			throw EmptyCommandException(getId());
			
		// break command string into a vector of arguments
		parseArguments();
	}
	
	// restart if the reactor was running
	if (was_running)
		start();
}
	
void ScriptReactor::updateVocabulary(const Vocabulary& v)
{
	// first update anything in the Reactor base class that might be needed
	ConfigWriteLock cfg_lock(*this);
	Reactor::updateVocabulary(v);

	if (m_input_codec_ptr)
		m_input_codec_ptr->updateVocabulary(v);

	if (m_output_codec_ptr)
		m_output_codec_ptr->updateVocabulary(v);
}
	
void ScriptReactor::updateCodecs(void)
{
	// check if a codec was deleted (if so, stop now!)
	if (! getCodecFactory().hasPlugin(m_input_codec_id)
		|| ! getCodecFactory().hasPlugin(m_output_codec_id))
	{
		stop();
	} else {
		// update the codec pointer
		ConfigWriteLock cfg_lock(*this);
		m_input_codec_ptr = getCodecFactory().getCodec(m_input_codec_id);
		m_output_codec_ptr = getCodecFactory().getCodec(m_output_codec_id);
	}
}
	
void ScriptReactor::process(const EventPtr& e)
{
	PION_ASSERT(m_input_stream_ptr);
	PION_ASSERT(m_input_codec_ptr);

	// lock mutex to ensure that only one Event may be written at a time
	boost::mutex::scoped_lock write_lock(m_write_mutex);
	
	// write the Event to the pipe
	m_input_codec_ptr->write(*m_input_stream_ptr, *e);
	if (! *m_input_stream_ptr)
		throw WriteToPipeException(getId());
		
	// unlock mutex after writing to pipe
	write_lock.unlock();

	// note: delivery to other reactors is handled by readEvents()
}

int ScriptReactor::checkForNewEvents(void)
{
	int return_code = -1;
	
#ifdef _MSC_VER

	// WaitForSingleObject from WINAPI
	// see http://msdn.microsoft.com/en-us/library/ms687032(VS.85).aspx
	switch ( WaitForSingleObject(m_output_pipe, 0) ) {
	case WAIT_ABANDONED:	// 0x00000080L
		break;	// do nothing (error)
	case WAIT_OBJECT_0:		// 0x00000000L
		return_code = 1;
		break;
	case WAIT_TIMEOUT:		// 0x00000102L
		return_code = 0;
		break;
	default:
		break;	// do nothing
	};
	
#else

	// note: these need to be reset before each select()
	fd_set read_fds;
	struct timeval timeout;

	// add script output descriptor to list to check
	FD_ZERO(&read_fds);
	FD_SET(m_output_pipe, &read_fds);
	
	// set to zero to avoid blocking
	// DO NOT use NULL for timeout; it blocks forever
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	return_code = ::select(m_output_pipe+1, &read_fds, NULL, NULL, &timeout);
	
#endif

	return return_code;
}

void ScriptReactor::readEvents(void)
{
	PION_LOG_DEBUG(m_logger, "Script reader thread is running: " << getId());

	try {
	
		const Event::EventType event_type(m_output_codec_ptr->getEventType());
		EventFactory event_factory;
		EventPtr event_ptr;
		bool event_read;

		PION_ASSERT(m_output_stream_ptr);
		PION_ASSERT(m_output_codec_ptr);
		
		while ( isRunning() ) {

			// wait for data to be available
			int data_status = checkForNewEvents();
			
			if (data_status == 1) {

				ConfigReadLock cfg_lock(*this);
				if ( ! isRunning() )	// re-check after acquiring lock
					break;

				// data is available for reading

				// get a new event from the EventFactory
				event_factory.create(event_ptr, event_type);
				
				// read an event using the output codec
				event_read = m_output_codec_ptr->read(*m_output_stream_ptr, *event_ptr);
	
				if (event_read) {
	
					// deliver the Event to other Reactors
					deliverEvent(event_ptr);
	
				} else {
				
					// check for read error
					throw ReadFromPipeException(getId());
				}
				
			} else if (data_status == 0) {

				// no data currently available
				PionScheduler::sleep(0, 100000000);
			
			} else {

				// error checking for data availability
				// ignore EINTR errors (receives signals on Linux)
				if (errno != 4)
					throw SelectPipeException(getId());
			}
		}

	} catch (std::exception& e) {
		ConfigWriteLock cfg_lock(*this);
		if ( isRunning() ) {
			PION_LOG_FATAL(m_logger, e.what());
			closePipe();
			m_is_running = false;
		}
	}

	PION_LOG_DEBUG(m_logger, "Script reader thread is exiting: " << getId());
}

void ScriptReactor::openPipe(void)
{
	// close first if already open
	closePipe();
	
	PION_LOG_DEBUG(m_logger, "Opening pipe to command: " << m_command);
	
	PION_ASSERT(! m_args.empty());

#ifdef _MSC_VER

	// create child process code adapted from:
	// http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx

	// prepare security attributes to use for creating pipes
	SECURITY_ATTRIBUTES sa_attr;
	sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa_attr.bInheritHandle = TRUE;
	sa_attr.lpSecurityDescriptor = NULL;

	// from int_tmain():
	HANDLE child_stdout_read = NULL;
	HANDLE child_stdout_write = NULL;
	HANDLE child_stdin_read = NULL;
	HANDLE child_stdin_write = NULL;
	
	// create pipe for child stdout
	if ( ! CreatePipe(&child_stdout_read, &child_stdout_write, &sa_attr, 0) )
		throw OpenPipeException(m_command);

	// ensure read handle for child stdout is not inherited
	if ( ! SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0) )
		throw OpenPipeException(m_command);

	// create pipe for child stdin
	if ( ! CreatePipe(&child_stdin_read, &child_stdin_write, &sa_attr, 0) )
		throw OpenPipeException(m_command);

	// ensure write handle for child stdin is not inherited
	if ( ! SetHandleInformation(child_stdin_write, HANDLE_FLAG_INHERIT, 0) )
		throw OpenPipeException(m_command);

	// from CreateChildProcess(), with some liberties...:
	char *command = _strdup(m_command.c_str());
	CLEAR_PROCESS_INFO(m_child);
	
	// prepare startup information for child process
	STARTUPINFO start_info;
	ZeroMemory( &start_info, sizeof(STARTUPINFO) );
	start_info.cb = sizeof(STARTUPINFO);
	start_info.hStdOutput = child_stdout_write;
	start_info.hStdInput = child_stdin_read;
	start_info.dwFlags |= STARTF_USESTDHANDLES;
	
	// create the child process
	BOOL result = CreateProcess(NULL,
		command,		// command line
		NULL,			// process security attributes
		NULL,			// primary thread security attributes
		TRUE,			// handles are inherited
		0,				// creation flags
		NULL,			// use parent's environment
		NULL,			// use parent's current directory
		&start_info,	// STARTUPINFO pointer
		&m_child);		// receives PROCESS_INFORMATION
	
	// check result of creating child process
	free(command);
	if (result) {
		m_input_pipe = child_stdin_write;
		m_output_pipe = child_stdout_read;
	} else {
		// close process and thread handles
		CloseHandle(m_child.hProcess);
		CloseHandle(m_child.hThread);
		throw OpenPipeException(m_command);
	}

#else

	// initialize pipes for reading and writing to script
	int r_pipes[2];
	int w_pipes[2];
	if ( ::pipe(r_pipes) != 0 || ::pipe(w_pipes) != 0)
		throw OpenPipeException(m_command);
	
	// fork child process
	m_child = ::fork();

	if (m_child == -1) {

		// failed to fork child process
		throw OpenPipeException(m_command);

	} else if (m_child == 0) {

		// inside child process

		// close ends of pipes used by parent
		CLOSE_DESCRIPTOR(w_pipes[1]);
		CLOSE_DESCRIPTOR(r_pipes[0]);

		// bind other ends of pipes to STDIN/STDOUT
		::dup2(w_pipes[0], 0);	// STDIN
		::dup2(r_pipes[1], 1);	// STDOUT
		CLOSE_DESCRIPTOR(w_pipes[0]);
		CLOSE_DESCRIPTOR(r_pipes[1]);
		
		// convert argument string vector into an array of char pointers
		boost::scoped_array<char*> arg_ptr(new char*[m_args.size() + 1]);
		for (std::size_t n = 0; n < m_args.size(); ++n) {
			arg_ptr[n] = const_cast<char*>(m_args[n].c_str());
		}
		arg_ptr[ m_args.size() ] = NULL;
		
		// execute command (ignore return since we're in a new process anyway)
		// note: execvp is wrapper for execve() that also searches the paths
		::execvp(m_args[0].c_str(), arg_ptr.get());

	} else {

		// inside parent process
		
		// close ends of pipes used by child
		CLOSE_DESCRIPTOR(w_pipes[0]);
		CLOSE_DESCRIPTOR(r_pipes[1]);

		// set file descriptors for reading/writing
		m_input_pipe = w_pipes[1];
		m_output_pipe = r_pipes[0];
	}

#endif

	// prepare c++ streams to use pipes for reading and writing events
	m_input_streambuf_ptr.reset(new StreamBuffer( m_input_pipe ));
	m_input_stream_ptr.reset(new std::ostream(m_input_streambuf_ptr.get()));
	m_output_streambuf_ptr.reset(new StreamBuffer( m_output_pipe ));
	m_output_stream_ptr.reset(new std::istream(m_output_streambuf_ptr.get()));
}

void ScriptReactor::closePipe(void)
{
	if (m_input_pipe != INVALID_DESCRIPTOR || m_output_pipe != INVALID_DESCRIPTOR) {
		PION_LOG_DEBUG(m_logger, "Closing pipe to command: " << m_command);

		// note: file descriptors are closed by the iostreams

		// close child process
#ifdef _MSC_VER
		CloseHandle(m_child.hProcess);
		CloseHandle(m_child.hThread);
#else
		::kill(m_child, SIGKILL);
#endif
		CLEAR_PROCESS_INFO(m_child);

		// close iostreams	
		m_input_stream_ptr.reset();
		m_output_stream_ptr.reset();
		m_input_streambuf_ptr.reset();
		m_output_streambuf_ptr.reset();

		// reset pipe handles
		m_input_pipe = m_output_pipe = INVALID_DESCRIPTOR;
	}
}

void ScriptReactor::parseArguments(void)
{
	std::string arg;			// current argument string
	std::size_t next_pos = 0;	// next parsing position
	char next_delimiter = ' ';	// next argument delimiter

	PION_LOG_DEBUG(m_logger, "Parsing out command string: " << m_command);

	// start by clearing the arguments vector
	m_args.clear();

	// step through each character in the command string
	while (next_pos < m_command.size()) {
		switch ( m_command[next_pos] ) {
		case '\\':
			if ( next_pos + 1 < m_command.size() &&
				(m_command[next_pos+1]==' ' || m_command[next_pos+1]=='\"'
				|| m_command[next_pos+1]=='\'') )
			{
				// escape next character
				arg.push_back( m_command[++next_pos] );
			} else {
				// append slash
				arg.push_back( m_command[next_pos] );
			}
			break;

		case ' ':
			if (next_delimiter == ' ') {
				if (! arg.empty()) {	// skip leading whitespace
					// finished with space-delimited argument
					m_args.push_back(arg);
					arg.clear();
				}
			} else {
				// append space -- not space-delimited
				arg.push_back(' ');
			}
			break;

		case '\'':
		case '\"':
			if (next_delimiter == m_command[next_pos]) {
				// finished with quote/tick-delimited argument
				m_args.push_back(arg);
				arg.clear();
				next_delimiter = ' ';
			} else if (arg.empty()) {
				// first char in argument: set delimiter
				next_delimiter = m_command[next_pos];
			} else {
				// append quote/tick -- not the delimiter or first character
				arg.push_back( m_command[next_pos] );
			}
			break;
			
		default:
			// append character
			arg.push_back( m_command[next_pos] );
			break;
		}
		
		++next_pos;
	}
	
	// push final argument if ending delimiter not found
	if (!arg.empty())
		m_args.push_back(arg);
		
	// sanity check: args should never be empty
	if (m_args.empty())
		throw CommandParsingException(m_command);
		
	// log each argument parsed for debugging
	for (std::vector<std::string>::const_iterator it = m_args.begin(); it != m_args.end(); ++it) {
		PION_LOG_DEBUG(m_logger, "Command string argument: " << *it);
	}
}

}	// end namespace plugins
}	// end namespace pion


/// creates new ScriptReactor objects
extern "C" PION_PLUGIN_API pion::platform::Reactor *pion_create_ScriptReactor(void) {
	return new pion::plugins::ScriptReactor();
}

/// destroys ScriptReactor objects
extern "C" PION_PLUGIN_API void pion_destroy_ScriptReactor(pion::plugins::ScriptReactor *reactor_ptr) {
	delete reactor_ptr;
}
