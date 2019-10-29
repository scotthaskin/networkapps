#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>


/// @brief This function is used to output an error based on a an error number
///
/// @param num - the error number to use
void outputError( int num, const char *data, int lineNumber )
{
    std::cerr << "[" << data << "] Failed with error number [" << num << "] [" << strerror(num) << "] [" << lineNumber << "]" << std::endl;
}

/// @brief This function is used to create a server socket for the echo service
///
/// @param port - the port number to bind to. A non-positive number indicates
///               that the default echo port should be used (i.e. 7)
///
/// @return the socket, or -1 on error
int createServerSocket( int port = -1 )
{
    int rval = -1;

    if( port < 0 )
    {
        // Using the default port number
        servent *serviceInfo = getservbyname( "echo", "tcp" );
        if( serviceInfo )
        {
            port = serviceInfo->s_port;
        }
        else
        {
            port = 7; // Default if echo/tcp is not found in the services file
        }
    }

    // Check to see if we have a valid port
    if( !port || port > USHRT_MAX )
    {
        std::cerr << "Invalid Port Number [" << port << "]" << std::endl;
        return -1;
    }

    protoent *protocol = getprotobyname( "tcp" );

    if( !protocol )
    {
        std::cerr << "Failed to get the [tcp] protocol value" << std::endl;
        return -1;
    }

    if( ( rval = socket( AF_INET, SOCK_STREAM, protocol->p_proto ) ) == -1 )
    {
        outputError( errno, "socket()", __LINE__ );
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;

    if( -1 == bind( rval, (const struct sockaddr *) &addr, sizeof(sockaddr_in) ) )
    {
        outputError( errno, "bind()", __LINE__ );
        close( rval );
        return -1;
    }

    if( -1 == listen( rval, 5 ) )
    {
        outputError( errno, "listen()", __LINE__ );
        close( rval );
        return -1;
    }

    return rval;
}

/// @brief This function is used to work the echo socket
///
/// @param socket - the socket to use
void runEchoServer( int socket )
{
    if( socket < 0 )
    {
        std::cerr << "Invalid Socket" << std::endl;
        return;
    }

    char buffer[1024];
    for(;;)
    {
        int fd = accept( socket, 0, 0 );
        if( fd == -1 )
        {
            outputError( errno, "accept()", __LINE__ );
            abort(); // Can't recover, goodbye
        }

        ssize_t size = -1;

        while( (size = read( fd, buffer, 1024 ) ) > 0 )
        {
            ssize_t loc = 0;
            do
            {
                loc += write( fd, &buffer[loc], size - loc );
            } while( loc < size );
        }

        if( size == -1 )
        {
            outputError( errno, "write()", __LINE__ );
        }

        (void) shutdown( fd, SHUT_RDWR );
        (void) close( fd );
    }
}

int main()
{
    runEchoServer( createServerSocket() );
}
