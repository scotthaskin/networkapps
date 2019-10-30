#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <iostream>

#define ERRNUM_OUTPUT(X) do { outputError( errno, X, __LINE__, __FILE__ ); } while( 0 )

/// @brief This function is used to output an error based on a an error number
///
/// @param num - the error number to use
/// @param data - a string to output
/// @param lineNumber - the line number that the error occured at
/// @param filename - the name of the file in which the error occured
void outputError( int num, const char *data, int lineNumber, const char *filename )
{
    if( data ) std::cerr << "[" << data << "] ";
    std::cerr << "Failed with error number [" << num << "] [" << strerror(num) << "] [" << lineNumber << "][" << filename << "]" << std::endl;
}

int main( int argc, char **argv )
{
    int arg = -1;
    unsigned port = -1;

    struct option options[] = {
        { "port", required_argument, 0, 'p' },
        { 0, 0, 0, 0 } // end of list
    };

    while( ( arg = getopt_long( argc, argv, "p:?", options, 0 ) ) != -1 )
    {
        switch( arg )
        {
            case 'p':
                if( optarg )
                {
                    port = htons( atoi( optarg ) );
                }
                break;
            default:
                std::cout << "Usage: " << argv[0] << " [--port|-p] {port number} " << std::endl;
                return 1;
        }
    }

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
            port = htons( 7 ); // Default if echo/tcp is not found in the services file
        }
    }

    // Check to see if we have a valid port
    if( !port || port > USHRT_MAX )
    {
        std::cerr << "Invalid Port Number [" << port << "]" << std::endl;
        return 1;
    }

    protoent *protocol = getprotobyname( "tcp" );

    if( !protocol )
    {
        std::cerr << "Failed to get the [tcp] protocol value" << std::endl;
        return 1;
    }

    // Setting server socket
    int sock = -1;
    if( ( sock = socket( AF_INET, SOCK_STREAM, protocol->p_proto ) ) == -1 )
    {
        ERRNUM_OUTPUT( "socket()" );
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;

    if( -1 == bind( sock, (const struct sockaddr *) &addr, sizeof(sockaddr_in) ) )
    {
        ERRNUM_OUTPUT( "bind()" );
        close( sock );
        return 1;
    }

    if( -1 == listen( sock, 5 ) )
    {
        ERRNUM_OUTPUT( "listen()" );
        close( sock );
        return 1;
    }

    // Running Server
    char buffer[1024];

    for(;;) // Forever
    {
        int fd = accept( sock, 0, 0 );
        if( fd == -1 )
        {
            ERRNUM_OUTPUT( "accept()" );
            return 1; // Can't recover, goodbye
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
            ERRNUM_OUTPUT( "write()" );
        }

        (void) shutdown( fd, SHUT_RDWR );
        (void) close( fd );
    }

    return 0;
}
