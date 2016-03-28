/*
 * qmguistatemachine.h
 *
 *  Created on: 23 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUISTATEMACHINE_H_
#define QMGUISTATEMACHINE_H_

#include <vector>


template<class States, class Events, class Context>
class StateMachine{
    public:
        StateMachine(){
            this->trans_count=0;
            init_flag=false;
        }

        void NextState(Events event){
            if(init_flag){
                for(int i=0;i<transitions.size();++i){
                    if(transitions[i].state==cur_state && transitions[i].event==event){
                        SetState(transitions[i].next_state);
                       	if(transitions[i].callback!=0){
                       		(context->*(transitions[i].callback))();
                       	}
                        return;
                    }
                }
            }
        }

        States GetState(){
            return cur_state;
        }
        States ResetMachine(){
            SetState(initial_state);
            return cur_state;
        }

        int AddTransition(States state, Events event, States next_state, void (Context::* callback)()){
        	if(init_flag){      //Добавление возможно только до инициализации
                return 1;
            }
        	Transition new_transition={.state=state,.event=event,.next_state=next_state,.callback=callback};
        	transitions.push_back(new_transition);
            return 0;
        }

        int InitStateMachine(States init_state, Context *object){
            if(init_flag){
                return 1;
            }

            for(int i=0;i<transitions.size();++i){     //Проверка корректности
                for(int j=i+1;j<transitions.size();++j){
                    if(transitions[i].state==transitions[j].state &&
                            transitions[i].event==transitions[j].event){
                    	QM_ASSERT(0);
                    	return 1;   //Ошибка: для состояния существует два перехода по одному и тому же событию
                    }
                }
            }
            this->initial_state=init_state;
            this->context=object;
            init_flag=true;
            ResetMachine();
            return 0;
        }

    private:
        struct Transition{
            States state;
            Events event;
            States next_state;
            void (Context::*callback)();    //Объявление колбека
        };


        bool init_flag;
        States initial_state;
        States cur_state;
        std::vector<Transition> transitions;
        Context *context;   //Указатель на объект контекста, нужен для вызова колбеков

        void SetState(States state){
            cur_state=state;
        }
};

#endif /* QMGUISTATEMACHINE_H_ */
