/*
 * MyDispatcherHandler.hpp
 *
 *  Created on: 2011-7-23
 *      Author: wqy
 */

#ifndef MYDISPATCHERHANDLER_HPP_
#define MYDISPATCHERHANDLER_HPP_
#include "framework/handler/dispatcher_handler.hpp"
using namespace arch::framework;



class MyDispatcherHandler: public DispatcherHandler
{
		bool OnInit();
		bool OnDestroy();
		/**
		 * ���м��ص�
		 */
		void OnRoutine();

};

#endif /* MYDISPATCHERHANDLER_HPP_ */
