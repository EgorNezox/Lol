#ifndef SHELDURER_H
#define SHELDURER_H

#include <qmobject.h>
#include <string.h>
#include <vector>

class Sheldurer
{
public:
    Sheldurer(int range);
    struct Seans{
        int key;
        int time;
        int frequency;
        int type;
    } seans_list;

    std::vector<Seans> list_communication;

    void CreateTask(int key, int time, int type, int freq);
    void CreateSheldure(int time, int type, int freq);
    void DeleteTask(int key);

    // ----  signals for GUI -----------
    sigc::signal<void> addTask;
    sigc::signal<void> deleteTask;
    sigc::signal<void> Create_sheldure;


private:
    int range;
};

#endif // SHELDURER_H
