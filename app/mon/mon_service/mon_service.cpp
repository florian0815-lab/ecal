/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2025 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

/**
 * @brief eCAL Service Monitor - CLI tool to monitor RPC services
**/

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <ecal/ecal.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4146 4505 4800 4189 4592)
#endif
#include "ecal/core/pb/monitoring.pb.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

void PrintUsage(const char* prog_name)
{
  std::cout << "Usage: " << prog_name << " [OPTIONS]\n\n"
            << "Options:\n"
            << "  --list-servers         List all registered RPC servers\n"
            << "  --list-clients         List all registered RPC clients\n"
            << "  --list-all             List both servers and clients\n"
            << "  --service <name>       Show details for specific service\n"
            << "  --check-service <name> Health check: exit 0 if service exists, 1 otherwise\n"
            << "  --continuous           Keep monitoring (default: single snapshot)\n"
            << "  --help                 Show this help message\n"
            << std::endl;
}

void PrintServer(const eCAL::pb::Service& service, bool detailed = false)
{
  std::cout << "Service (Server): " << service.service_name() << "\n";
  std::cout << "  Host:       " << service.host_name() << "\n";
  std::cout << "  Process:    " << service.process_name() << " (PID: " << service.process_id() << ")\n";
  std::cout << "  Service ID: " << service.service_id() << "\n";
  std::cout << "  Version:    " << service.version() << "\n";
  
  if (service.tcp_port_v0() > 0)
    std::cout << "  TCP Port v0: " << service.tcp_port_v0() << "\n";
  if (service.tcp_port_v1() > 0)
    std::cout << "  TCP Port v1: " << service.tcp_port_v1() << "\n";
  
  if (detailed && service.methods_size() > 0)
  {
    std::cout << "  Methods (" << service.methods_size() << "):\n";
    for (const auto& method : service.methods())
    {
      std::cout << "    - " << method.method_name() 
                << " (calls: " << method.call_count() << ")\n";
      if (!method.req_type().empty())
      {
        std::cout << "      Request:  " << method.req_type() << "\n";
        std::cout << "      Response: " << method.resp_type() << "\n";
      }
    }
  }
  std::cout << std::endl;
}

void PrintClient(const eCAL::pb::Client& client, bool detailed = false)
{
  std::cout << "Service (Client): " << client.service_name() << "\n";
  std::cout << "  Host:       " << client.host_name() << "\n";
  std::cout << "  Process:    " << client.process_name() << " (PID: " << client.process_id() << ")\n";
  std::cout << "  Service ID: " << client.service_id() << "\n";
  std::cout << "  Version:    " << client.version() << "\n";
  
  if (detailed && client.methods_size() > 0)
  {
    std::cout << "  Methods (" << client.methods_size() << "):\n";
    for (const auto& method : client.methods())
    {
      std::cout << "    - " << method.method_name() << "\n";
    }
  }
  std::cout << std::endl;
}

int main(int argc, char** argv)
{
  // Parse arguments
  bool list_servers = false;
  bool list_clients = false;
  bool continuous = false;
  bool check_mode = false;
  std::string service_filter;
  
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    
    if (arg == "--help" || arg == "-h")
    {
      PrintUsage(argv[0]);
      return 0;
    }
    else if (arg == "--list-servers")
    {
      list_servers = true;
    }
    else if (arg == "--list-clients")
    {
      list_clients = true;
    }
    else if (arg == "--list-all")
    {
      list_servers = true;
      list_clients = true;
    }
    else if (arg == "--service" && i + 1 < argc)
    {
      service_filter = argv[++i];
      list_servers = true;
      list_clients = true;
    }
    else if (arg == "--check-service" && i + 1 < argc)
    {
      service_filter = argv[++i];
      check_mode = true;
      list_servers = true;
    }
    else if (arg == "--continuous")
    {
      continuous = true;
    }
    else
    {
      std::cerr << "Unknown option: " << arg << "\n\n";
      PrintUsage(argv[0]);
      return 1;
    }
  }
  
  // Default: list all
  if (!list_servers && !list_clients)
  {
    list_servers = true;
    list_clients = true;
  }
  
  // Initialize eCAL
  auto config = eCAL::Init::Configuration();
  eCAL::Initialize(config, "mon_service", eCAL::Init::All);
  
  // Give eCAL time to discover services
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  
  bool found_service = false;
  
  do
  {
    // Get monitoring snapshot
    std::string monitoring_str;
    eCAL::pb::Monitoring monitoring;
    
    if (eCAL::Monitoring::GetMonitoring(monitoring_str))
    {
      monitoring.ParseFromString(monitoring_str);
      
      if (!check_mode)
      {
        // List servers
        if (list_servers)
        {
          for (const auto& service : monitoring.services())
          {
            if (service_filter.empty() || service.service_name() == service_filter)
            {
              PrintServer(service, !service_filter.empty());
              found_service = true;
            }
          }
        }
        
        // List clients
        if (list_clients)
        {
          for (const auto& client : monitoring.clients())
          {
            if (service_filter.empty() || client.service_name() == service_filter)
            {
              PrintClient(client, !service_filter.empty());
              found_service = true;
            }
          }
        }
        
        if (!service_filter.empty() && !found_service)
        {
          std::cout << "Service '" << service_filter << "' not found." << std::endl;
        }
      }
      else
      {
        // Health check mode
        for (const auto& service : monitoring.services())
        {
          if (service.service_name() == service_filter)
          {
            found_service = true;
            break;
          }
        }
      }
    }
    
    if (continuous && !check_mode)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    else
    {
      break;
    }
    
  } while (eCAL::Ok() && continuous);
  
  // Finalize eCAL
  eCAL::Finalize();
  
  // Return appropriate exit code for health checks
  if (check_mode)
  {
    return found_service ? 0 : 1;
  }
  
  return 0;
}
