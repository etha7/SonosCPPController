#include "main.hpp"

#define  ERR_NO_SPEAKER "Use command 'discover' to find a Sonos speaker on your local network\n"

#define  MULTICAST_IP "239.255.255.250"
#define  MULTICAST_PORT_REMOTE 1900
#define  MULTICAST_PORT_LOCAL 1901
#define  MULTICAST_MESSAGE "M-SEARCH * HTTP/1.1\r\nHOST: "MULTICAST_IP"\r\nMAN: \"ssdp:discover\"\r\nMX: 1\r\n"\
                           "ST: upnp:rootdevice\r\n\r\n"
#define  MAX_MULTICAST_PORT 2100
#define  TERMINAL_STRING "Sonos_Controller-:"


void sendUDP(boost::asio::ip::udp::socket *& socket, std::string ip , int port, std::string buffer){
          
    using boost::asio::ip::udp; 
    udp::endpoint remote(boost::asio::ip::address::from_string(ip.c_str()), port);
    socket->send_to(boost::asio::buffer(buffer.c_str(), strlen(buffer.c_str())), remote);
}

void bindToOpenMulticastPort(boost::asio::ip::udp::socket *& socket, boost::asio::ip::udp::endpoint *& local, 
                    boost::asio::io_service *& ioservice, int minPort, int maxPort){

           using boost::asio::ip::udp;

           int currentPort = minPort;
           while(currentPort < maxPort)
           {
             //Test a given port
             ioservice = new boost::asio::io_service();
             local = new udp::endpoint(udp::v4(), currentPort);
             socket = new udp::socket(*ioservice);
             boost::system::error_code ec;
             socket->open(udp::v4());
             socket->bind(*local, ec);
            
             //Port is free to use
             if(ec != boost::asio::error::address_in_use)
             {
                return;
             }
             else{
                currentPort++; 
             }
           }
           
        throw std::runtime_error("Couldn't find open port.\n");
               
}

std::string findSonosIP(boost::asio::ip::udp::socket *& socket, boost::asio::ip::udp::endpoint*& local){

        //Search for the Sonos speaker. 
        bool notFound = true;
        while(notFound)
        {
            char receive[256];
            socket->receive_from(boost::asio::buffer(receive),*local); 
            std::string receiveString(receive);\

            //Specifics of boost regex mandate double escaped \d 
            boost::regex e("LOCATION: http:\/\/(\\d*\.\\d*\.\\d*\.\\d*):(\\d*).*Sonos.*");
            boost::smatch matches;
             
            if(boost::regex_search(receiveString,matches,e))
            {
                return  matches[1]+":"+matches[2];
            }
        }
        throw std::runtime_error("Couldn't find Sonos speaker.\n");
}


std::string discover()
{
        try
        {
           using boost::asio::ip::udp;

           udp::endpoint* local;
           udp::socket* socket;
           boost::asio::io_service* ioservice; 

           bindToOpenMulticastPort(socket, local, ioservice, MULTICAST_PORT_LOCAL, MAX_MULTICAST_PORT);
           sendUDP(socket, MULTICAST_IP, MULTICAST_PORT_REMOTE, MULTICAST_MESSAGE);
           std::string ip = findSonosIP(socket, local);
            
           return ip;
        }
        catch (std::exception& e)
        {
           std::cerr << e.what() << std::endl;
        } 
        
}


//Turn input into a stream
template<typename T>
std::string toString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

int main()
{

   std::cout << "\nWelcome to the Sonos Controller!\n";
   std::cout << TERMINAL_STRING;

   //Initialize HTTP messanging object
   HttpMessenger * messenger = new HttpMessenger();

   std::string ip;

   //Header
   std::vector<std::string>* head = new std::vector<std::string>();
   head->push_back("CONNECTION: close");
   head->push_back("ACCEPT-ENCODING: gzip");
   head->push_back("HOST: 192.168.1.143:1400");
   head->push_back("USER-AGENT: Linux UPnP/1.0 Sonos/32.11-30162 (WDCR:Microsoft Windows NT 6.1.7601 Service Pack 1)");
   head->push_back("CONTENT-TYPE: text/xml;charset=\"utf-8\"");
   head->push_back("X-SONOS-TARGET-UDN: uuid:RINCON_5CAAFD793EDC01400");
  
   char * body;
   std::string url;
   std::vector<std::string>* headers;

   bool sonosDiscovered = false;

   std::string input;
   std::getline(std::cin,input);
   while(input.compare("quit") != 0){

     bool isPostCommand = true; 
     headers = new std::vector<std::string>(*head);
    
     if(input.compare("discover") != 0 && !sonosDiscovered)
     {
        std::cerr << ERR_NO_SPEAKER;
        isPostCommand = false;
     }

     if(input.compare("play") == 0)
     {
        body = BODY_PLAY;
        headers->push_back("CONTENT-LENGTH: "+ toString(strlen(body)));
        headers->push_back("SOAPACTION: \"urn:schemas-upnp-org:service:AVTransport:1#Play\"");

        //initialize url
        url = "http://"+ip;
        std::string path = URL_AV;
        url = url + path;
     }
     else if(input.compare("pause") == 0)
     {
        
        body = BODY_PAUSE;
        headers->push_back("CONTENT-LENGTH: "+ toString(strlen(body)));
        headers->push_back("SOAPACTION: \"urn:schemas-upnp-org:service:AVTransport:1#Pause\"");

        //initialize url
        url = "http://"+ip;
        std::string path = URL_AV;
        url = url + path;
        
     }
     else if(input.compare("previous") == 0)
     {
        
        body = BODY_PREVIOUS;
        headers->push_back("CONTENT-LENGTH: "+ toString(strlen(body)));
        headers->push_back("SOAPACTION: \"urn:schemas-upnp-org:service:AVTransport:1#Previous\"");

        //initialize url
        url = "http://"+ip;
        std::string path = URL_AV;
        url = url + path;
        
     }
     else if(input.compare("next") == 0)
     {
        
        body = BODY_NEXT;
        headers->push_back("CONTENT-LENGTH: "+ toString(strlen(body)));
        headers->push_back("SOAPACTION: \"urn:schemas-upnp-org:service:AVTransport:1#Next\"");

        //initialize url
        url = "http://"+ip;
        std::string path = URL_AV;
        url = url + path;
        
     }
     else if(input.compare("discover") == 0 )
     { 
        isPostCommand = false;
        ip = discover();
        sonosDiscovered = true;
     }
     else
     {
        isPostCommand = false;
     }
 
     if(isPostCommand)
     {
        //Send command to speaker.             
        messenger->post(headers, body, url);
     }
        std::cout << TERMINAL_STRING;
        std::getline(std::cin, input);
     
   }
}
