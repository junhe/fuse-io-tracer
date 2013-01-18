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

        // initial these before playing
        int _sleeptime;
        int _customized_sleeptime; // 0 or 1
        int _do_pread; // 0 or 1
        string _trace_path;
        string _data_path;

        void readTrace();
        void play();
        double playTime();
        void prePlay();
        void postPlay();
        void prefetch();
};

void Replayer::prefetch()
{
    assert( !_trace.empty() );

    vector<Entry>::const_iterator cit;

    for ( cit = _trace.begin();
          cit != _trace.end();
          cit++ )
    {
        posix_fadvise( _fd, cit->_offset, cit->_length, POSIX_FADV_WILLNEED);
    }
}

void Replayer::prePlay()
{
     _fd = open( _data_path.c_str(), O_RDONLY );
    assert( _fd != -1 );
}

void Replayer::postPlay()
{
    close(_fd);
}

void Replayer::readTrace()
{
    FILE *fp;
    

    fp = fopen(_trace_path.c_str(), "r");
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
        
        // start time
        char tmpchar[64];
        size_t pos;
        string tmpstr;
       
        // This part is only good for old FUSE tracer
        // where the time printed out by %lld.%lld
        ret += fscanf(fp, "%s", tmpchar);
        tmpstr.assign(tmpchar);
        
        pos = tmpstr.find(".");
        assert(pos != string::npos);
       
        while ( tmpstr.size() - pos < 7 ) {
            tmpstr.insert(pos+1, "0");
        }
        entry._start_time = atof(tmpstr.c_str());

        ret += fscanf(fp, "%s", tmpchar);
        tmpstr.assign(tmpchar);
        
        pos = tmpstr.find(".");
        assert(pos != string::npos);
        
        while ( tmpstr.size() - pos < 7 ) {
            tmpstr.insert(pos+1, "0");
        }
        entry._end_time = atof(tmpstr.c_str());


        if ( ret != 7 ) {
            break;
        }
        
        _trace.push_back( entry );
    }
    fclose(fp);
}


// TODO: the trace should have only one filepath column
void Replayer::play()
{
    assert( !_trace.empty() );

    vector<Entry>::const_iterator cit;
    int total = 0;


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
            assert( sleeptime >= 0 );
            if ( _customized_sleeptime == 1 ) {
                usleep( _sleeptime );
            } else {
                usleep( sleeptime );
            }
        }

        if ( _do_pread == 1 ) {
            total += pread(_fd, data, cit->_length, cit->_offset);
        }

        free(data);
    }

    cout << total << " " ;// "(" << total/1024 << "KB)" << endl;
}


double Replayer::playTime()
{
    struct timeval start, end, result;  

    // build trace
    readTrace();

    prePlay();

    prefetch();

    // play trace and time it
    gettimeofday(&start, NULL);
    play();
    gettimeofday(&end, NULL);

    postPlay();

    timersub( &end, &start, &result );
    printf("%ld.%.6ld\n", result.tv_sec, result.tv_usec);

    return result.tv_sec + result.tv_usec/1000000;
}

int main(int argc, char **argv)
{
    if ( argc != 6 ) {
        printf("Usage: %s trace-file output-file sleeptime customized-sleeptime do-pread\n", argv[0]);
        return 1;
    }

    Replayer replayer;

    // init
    replayer._trace_path =  argv[1];
    replayer._data_path = argv[2];
    replayer._sleeptime = atoi(argv[3]);
    replayer._customized_sleeptime = atoi(argv[4]);
    replayer._do_pread = atoi(argv[5]);


    cout << replayer._trace_path << " ";
    cout << replayer._data_path << " ";
    cout << replayer._sleeptime << " ";
    cout << replayer._customized_sleeptime << " ";
    cout << replayer._do_pread << " ";


    replayer.playTime();
    return 0;    
}

