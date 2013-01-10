#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>



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
        
        void show();
};

void Entry::show()
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
        void play();
};

void Replayer::readTrace(const char *fpath)
{
    FILE *fp;
    int cnt = 0;

    fp = fopen(fpath, "r");
    assert(fp != NULL);

    while ( !feof(fp) ) {
        Entry entry;

        cout << "cnt:" << cnt++ << endl;

        char path[256];
        char operation[256];
        fscanf(fp, "%u", &entry._pid);
        fscanf(fp, "%s", path);
        entry._path.assign(path);
        fscanf(fp, "%s", operation);
        entry._operation.assign(operation);
        fscanf(fp, "%lld", &entry._offset);
        fscanf(fp, "%d", &entry._length);
        fscanf(fp, "%lf", &entry._start_time);
        fscanf(fp, "%lf", &entry._end_time);
        entry.show();
        _trace.push_back( entry );
    }
     
    fclose(fp);
}

void Replayer::play()
{
    assert( !_trace.empty() );

    vector<Entry>::const_iterator cit;

    _fd = open( _trace.front()._path.c_str(), O_RDONLY );
    
    for ( cit = _trace.begin();
          cit != _trace.end();
          cit++ )
    {
        void *data = malloc(cit->_length);
        assert( data != NULL );

        ssize_t ret = pread(_fd, data, cit->_length, cit->_offset);

        free(data);
    }


    close( _fd );
}

int main(int argc, char **argv)
{
    Replayer replayer;
    replayer.readTrace(argv[1]);
    return 0;    
}

