#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// --- STRUCT ---
struct ParsedCommand
{
	std::vector<std::vector<std::string>> pipeline;
	std::string inputFile;
	std::string outputFile;
	bool background = false;
};

// --- PARSER ---
ParsedCommand parseCommandLine(const std::string& line)
{
	ParsedCommand res;
	std::istringstream iss(line);
	std::string token;
	std::vector<std::string> currentCmd;

	while(iss >> token)
	{
		if(token == "|")
		{
			if(!currentCmd.empty())
			{
				res.pipeline.push_back(currentCmd);
				currentCmd.clear();
			}
		}
		else if(token == "<")
		{
			iss >> res.inputFile;
		}
		else if(token == ">")
		{
			iss >> res.outputFile;
		}
		else if(token == "&")
		{
			res.background = true;
		}
		else
		{
			currentCmd.push_back(token);
		}
	}

	if(!currentCmd.empty())
	{
		res.pipeline.push_back(currentCmd);
	}

	return res;
}

// --- LAUNCH ---
int launch(const ParsedCommand& cmd)
{
	size_t numCommands = cmd.pipeline.size();
	if(numCommands == 0)
		return 1;

	// --- Special Case: cd ---
	if(numCommands == 1 && cmd.pipeline[0][0] == "cd")
	{
		const char* path =
			cmd.pipeline[0].size() > 1 ? cmd.pipeline[0][1].c_str() : getenv("HOME");
		if(chdir(path) != 0)
		{
			perror("cd");
		}
		return 1;
	}

	// --- обычные команды с pipe ---
	std::vector<int[2]> pipes(numCommands - 1);

	// pipe() create
	for(size_t i = 0; i < numCommands - 1; ++i)
	{
		if(pipe(pipes[i]) == -1)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}
	}

	// fork + exec
	for(size_t i = 0; i < numCommands; ++i)
	{
		pid_t pid = fork();
		if(pid == 0)
		{
			// input redirect
			if(i == 0 && !cmd.inputFile.empty())
			{
				int fd = open(cmd.inputFile.c_str(), O_RDONLY);
				if(fd == -1)
				{
					perror("open input");
					exit(EXIT_FAILURE);
				}
				dup2(fd, STDIN_FILENO);
				close(fd);
			}

			// output redirect
			if(i == numCommands - 1 && !cmd.outputFile.empty())
			{
				int fd = open(cmd.outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if(fd == -1)
				{
					perror("open output");
					exit(EXIT_FAILURE);
				}
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}

			// pipe: stdin from prev
			if(i > 0)
			{
				dup2(pipes[i - 1][0], STDIN_FILENO);
			}

			// pipe: stdout to next
			if(i < numCommands - 1)
			{
				dup2(pipes[i][1], STDOUT_FILENO);
			}

			// close all pipes in child
			for(auto& p : pipes)
			{
				close(p[0]);
				close(p[1]);
			}

			// execvp
			std::vector<char*> c_args;
			for(const auto& arg : cmd.pipeline[i])
				c_args.push_back(const_cast<char*>(arg.c_str()));
			c_args.push_back(nullptr);

			execvp(c_args[0], c_args.data());
			perror("execvp");
			exit(EXIT_FAILURE);
		}
		else if(pid < 0)
		{
			perror("fork");
		}
	}

	// close all pipes in parent
	for(auto& p : pipes)
	{
		close(p[0]);
		close(p[1]);
	}

	// wait for children (if not background)
	if(!cmd.background)
	{
		int status;
		while(wait(&status) > 0)
			;
	}

	return 1;
}

// --- SHELL LOOP ---
void shellLoop()
{
	while(true)
	{
		std::cout << "$ ";
		std::string line;
		if(!std::getline(std::cin, line))
			break;

		if(line == "exit")
			break;

		ParsedCommand cmd = parseCommandLine(line);
		launch(cmd);
	}
}

// --- MAIN ---
int main()
{
	shellLoop();
	return 0;
}
