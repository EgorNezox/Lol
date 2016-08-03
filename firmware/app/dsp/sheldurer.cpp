#include "sheldurer.h"


/* Класс предназначен для планирования сеансов связи по заданным условиям
 * Класс ориентируется на календарь
 * Den
*/

Sheldurer::Sheldurer(int range)
{
    this->range = range;
}

void Sheldurer::CreateTask(int key, int time, int type, int freq)
{
    seans_list.key  = key;
    seans_list.time = time;
    seans_list.type = type;
    seans_list.frequency = freq;

    list_communication.push_back(seans_list);
    addTask();
}

void Sheldurer::DeleteTask(int key)
{
    for(int i = 0; i< list_communication.size();i++){
        if ( ( (Seans)(list_communication.at(i)) ).key == key)
            list_communication.erase(list_communication.begin() + i);
    }

   deleteTask();
}

void Sheldurer::CreateSheldure(int time, int type, int freq)
{
    int index = 0;
    for (int i = 0; i< range; i += time){
        CreateTask(index,i, index % 4, freq);
        ++index;
    }

    Create_sheldure();
}

