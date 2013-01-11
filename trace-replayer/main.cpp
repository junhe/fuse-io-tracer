#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>


using namespace std;

class Entry {
    public:
        pid_t  _pid;
        string _path;
        string _operation;
        off_t  _offset;
        int    _length;
        double _start_time;
        double _end_time;
        
        void show() const;
};

void Entry::show() const
{
    cout << _pid << " "
         << _path << " "
         << _operation << " "
         << _offset << " "
         << _length << " "
         << _start_time << " "
         << _end_time << endl;
}

class Replayer{
    public:
        vector<Entry> _trace;
        int _fd;

        void readTrace(const char *fpath);
        void play(const char *outpath);
        double playTime( const char *tracepath,
                           const char *outpath);
};

void Replayer::readTrace(const char *fpath)
{
    FILE *fp;

    fp = fopen(fpath, "r");
    assert(fp != NULL);

    while ( !feof(fp) ) {
        Entry entry;

        char path[256];
        char operation[256];
        int ret = 0;
        ret += fscanf(fp, "%u", &entry._pid);
        ret += fscanf(fp, "%s", path);
        
        entry._path.assign(path);
        
        ret += fscanf(fp, "%s", operation);
        
        entry._operation.assign(operation);
        ret += fscanf(fp, "%lld", &entry._offset);
        ret += fscanf(fp, "%d", &entry._length);
        ret += fscanf(fp, "%lf", &entry._start_time);
        ret += fscanf(fp, "%lf", &entry._end_time);

        if ( ret != 7 ) {
            break;
        }

        _trace.push_back( entry );
    }
     
    fclose(fp);
}


// TODO: the trace should have only one filepath column
void Replayer::play(const char *outpath)
{
    assert( !_trace.empty() );

    vector<Entry>::const_iterator cit;

    _fd = open( outpath, O_RDONLY );
    assert( _fd != -1 );

    for ( cit = _trace.begin();
          cit != _trace.end();
          cit++ )
    {
        void *data = malloc(cit->_length);
        assert( data != NULL );

        if ( cit != _trace.begin() ) {
            // sleep to simulate computation
            vector<Entry>::const_iterator precit;
            precit = cit;
            precit--;
            int sleeptime = (cit->_start_time - precit->_end_time) * 1000000;
            //usleep( sleeptime );
            if ( sleeptime < 0 ) {
                cout << "sleeptime: " << sleeptime/1000000.0 << endl;
                cit->show();
            }
        }

        //cit->show();
        pread(_fd, data, cit->_length, cit->_offset);
        

        free(data);
    }


    close( _fd );
}


double Replayer::playTime( const char *tracepath,
                           const char *outpath)
{
    struct timeval start, end, result;  

    // build trace
    readTrace(tracepath);

    // play trace and time it
    gettimeofday(&start, NULL);
    play(outpath);
    gettimeofday(&end, NULL);

    timersub( &end, &start, &result );
    printf("Time: %ld.%ld\n", result.tv_sec, result.tv_usec);

    return result.tv_sec + result.tv_usec/1000000;
}

int main(int argc, char **argv)
{
    if ( argc != 3 ) {
        printf("Usage: %s trace-file output-file\n", argv[0]);
        return 1;
    }

    Replayer replayer;
    replayer.playTime(argv[1], argv[2]);

    return 0;    
}

