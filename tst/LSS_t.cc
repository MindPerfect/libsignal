/** \file LSS_t.cc 
 * Test definitions for the lss basic tests.
 *
 * (c) Copyright by MindPerfect Technologies
 * All rights reserved, see COPYRIGHT file for details.
 *
 * $Id$
 *
 *
 */

#include "lss.hh"
// Std includes
// Google Test
#include <gtest/gtest.h>
#include <absl/log/log.h>

using namespace std;
using namespace lss;


struct SignalOwner : public lss::slot
{
};


TEST(lss_lock, Base)
{
	lss::signal<void()> sg;
	ASSERT_TRUE(sg.is_locked()==false); // Signal should be unlocked.
	
	sg.set_lock(true);
	ASSERT_TRUE(sg.is_locked()==true); // Signal should be locked.

	sg.set_lock(false);
	ASSERT_TRUE(sg.is_locked()==false); // Signal should be unlocked.
	
	sg.set_lock(true);
	EXPECT_TRUE(sg.is_locked()==true) << "Custom message: Checking if value is locked."; 
	//LOG(INFO) << "Custom message: Checking if value is locked.";
}

  
TEST(lss_singleconn, Base)  // Single connection
{
	lss::signal<void(int, bool)> sg;

	int paramOne = 7;
	bool paramTwo = true;
	bool receiverCalled = false;

	std::function<void(int, bool)> receiver = [=, &receiverCalled](int p0, bool p1)
	{
		receiverCalled = true;
		ASSERT_TRUE(p0==paramOne); // First parameter should be as expected
		ASSERT_TRUE(p1==paramTwo); // Second parameter should be as expected
	};

	sg.connect(receiver, nullptr);
	
	sg(paramOne, paramTwo); // Fire signal

	ASSERT_TRUE(receiverCalled); // Receiver should be called
}


TEST(lss_multiconn, Base)  // Multi-connection
{

	lss::signal<void(int, bool)> sg;

	int paramOne = 7;
	bool paramTwo = true;
	unsigned char receiverCalledTimes = 0;

	std::function<void(int, bool)> receiver = [=, &receiverCalledTimes](int p0, bool p1)
	{
		++receiverCalledTimes;
		ASSERT_TRUE(p0==paramOne); // First parameter should be as expected
		ASSERT_TRUE(p1==paramTwo); // Second parameter should be as expected
	};

	sg.connect(receiver, nullptr);
	sg.connect(receiver, nullptr);

	sg(paramOne, paramTwo);  // Fire signal

	ASSERT_TRUE(receiverCalledTimes==2); // Count of calls of receiver should be as expected
}


TEST(lss_noconn, Base)  // No connection
{
	lss::signal<void(int, bool)> sg;

	int paramOne = 7;
	bool paramTwo = true;

	sg(paramOne, paramTwo); // Fire signal
	ASSERT_TRUE(true); // TODO: More meaningful test
}


TEST(lss_oneOwnerMultiSignals, Base)  // Same owner multi signals
{
	lss::signal<void()> sigOne;
	lss::signal<void()> sigTwo;

	bool receiverOneCalled = false;
	bool receiverTwoCalled = false;

	std::function<void()> receiverOne = [&receiverOneCalled]()
	{
		receiverOneCalled = true;
	};

	std::function<void()> receiverTwo = [&receiverTwoCalled]()
	{
		receiverTwoCalled = true;
	};

	{
		SignalOwner signalOwner;

		sigOne.connect(receiverOne, &signalOwner);
		sigTwo.connect(receiverTwo, &signalOwner);

		sigOne();
		sigTwo();

		ASSERT_TRUE(receiverOneCalled); // First receiver should be called.
		ASSERT_TRUE(receiverTwoCalled); // Second receiver should be called.
	}

	receiverOneCalled = false;
	receiverTwoCalled = false;

	sigOne();
	sigTwo();

	ASSERT_TRUE(receiverOneCalled==false); // First receiver should not be called.
	ASSERT_TRUE(receiverTwoCalled==false); // Second receiver should not be called.
	
}




// Test class signal/slot

static int receiveSigACount;
static int receiveSigBCount;
static bool connectionAddedInCallbackCalled;

class TestA : public slot
{
public:
	lss::signal<void(int data)> sigA;

	void ReceiveSigB(const std::string& data)
	{
		dataB = data;
		receiveSigBCount++;
	}

	void ReceiveDeleteSelf(int data)
	{
		(void)data;
		std::cout << "TestA::ReceiveDeleteSelf started" << std::endl;
		delete this;
		std::cout << "TestA::ReceiveDeleteSelf completed" << std::endl;
	}

	void AddConnectionInCallbackA(int data)
	{
		std::cout << "AddConnectionInCallbackA = " << data << std::endl;
		sigA.connect(this, &TestA::ConnectionAddedInCallback, this);
		receiveSigACount++;
	}

	void ConnectionAddedInCallback(int data)
	{
		(void)data;
		std::cout << "ConnectionAddedInCallback" << std::endl;
		connectionAddedInCallbackCalled = true;
		receiveSigACount++;
	}

	std::string dataB;
};


class TestB : public slot
{
public:
	lss::signal<void(const std::string& data)> sigB;

	void ReceiveSigA(int data)
	{
		//std::cout << "ReceiveSigA = " << data << std::endl;
		dataA = data;
		receiveSigACount++;
	}

	int dataA;

	connection explicitConnectionA;
};


TEST(lss_signalInClass, Base) 
{
	int dataA = 1023;
	std::string dataB = "DFBZ12Paql";
	TestA ta;
	TestB tb;

	ta.sigA.connect(&tb, &TestB::ReceiveSigA, &tb);
	tb.sigB.connect(&ta, &TestA::ReceiveSigB, &ta);

	ta.sigA(dataA);
	tb.sigB(dataB);

	ASSERT_TRUE(tb.dataA == dataA); // Verify dataA
	ASSERT_TRUE(ta.dataB == dataB); // Verify dataB

}


TEST(lss_signalDestroyListener, Base)  // TestSignalDestroyListener
{
	std::vector<TestB*> listeners;
	TestA ta;
	const size_t count = 1000;
	for (size_t i = 0; i < count; i++)
	{
		TestB* pb = new TestB();
		listeners.push_back(pb);
		ta.sigA.connect(pb, &TestB::ReceiveSigA, pb);
	}

	receiveSigACount = 0;
	ta.sigA(23);
	
	ASSERT_TRUE(count==receiveSigACount); // Verify siga count1
	//-AssertHelper::VerifyValue(count, receiveSigACount, "Verify siga count1");

	for (size_t i = 0; i < count; i += 2)
	{
		delete listeners[i];
		listeners[i] = nullptr;
	}

	receiveSigACount = 0;
	ta.sigA(34);
	
	ASSERT_TRUE((count / 2) == receiveSigACount); // Verify siga count/2
	//-AssertHelper::VerifyValue(count / 2, receiveSigACount, "Verify siga count/2");

	receiveSigACount = 0;
	ta.sigA.disconnect_all();
	ta.sigA(45);
	
	ASSERT_TRUE(0==receiveSigACount); // Verify disconnect_all
	//-AssertHelper::VerifyValue(0, receiveSigACount, "Verify disconnect_all");

	for (TestB* pb : listeners)
		delete pb;
}


TEST(lss_disconnectConnection, Base)  // TestDisconnectConnection
{
	std::vector<TestB*> listeners;
	TestA ta;
	const size_t count = 1000;
	for (size_t i = 0; i < count; i++)
	{
		TestB* pb = new TestB();
		listeners.push_back(pb);
		pb->explicitConnectionA = ta.sigA.connect(pb, &TestB::ReceiveSigA, pb);
	}

	receiveSigACount = 0;
	ta.sigA(23);
	
	ASSERT_TRUE(receiveSigACount == count); // Verify siga count1
	//-AssertHelper::VerifyValue(true, receiveSigACount == count, "Verify siga count1");

	for (size_t i = 0; i < count; i += 2)
	{
		ta.sigA.disconnect(listeners[i]->explicitConnectionA);
	}

	receiveSigACount = 0;
	ta.sigA(34);
	
	ASSERT_TRUE(receiveSigACount == (count / 2)); // Verify siga count/2
	//-AssertHelper::VerifyValue(true, receiveSigACount == count / 2, "Verify siga count/2");

	receiveSigACount = 0;
	for (size_t i = 0; i < count; i++)
	{
		ta.sigA.disconnect(listeners[i]->explicitConnectionA);
	}

	ta.sigA(45);
	ASSERT_TRUE(receiveSigACount == 0); // Verify disconnect_all
	//-AssertHelper::VerifyValue(true, receiveSigACount == 0, "Verify disconnect_all");

	for (TestB* pb : listeners)
		delete pb;
}


TEST(lss_destroySignal, Base)
{
	std::vector<TestB*> listeners;

	{
		TestA ta;
		const size_t count = 1000;
		for (size_t i = 0; i < count; i++)
		{
			TestB* pb = new TestB();
			listeners.push_back(pb);
			ta.sigA.connect(pb, &TestB::ReceiveSigA, pb);
		}

		for (size_t i = 0; i < count; i += 2)
		{
			ta.sigA.disconnect(listeners[i]->explicitConnectionA);
		}

		for (size_t i = 0; i < count; i += 3)
		{
			delete listeners[i];
			listeners[i] = nullptr;
		}
	}
	
	ASSERT_TRUE(true);

	for (TestB* pb : listeners)
		delete pb;
}


TEST(lss_signalSelfDelete, Base)
{
	TestA* pa = new TestA();
	pa->sigA.connect(pa, &TestA::ReceiveDeleteSelf, pa);
	pa->sigA(37);
	ASSERT_TRUE(true);
}


TEST(lss_signalCopy, Base)
{
	lss::signal<void(int, bool)> sg;

	int paramOne = 7;
	bool paramTwo = true;

	bool receiverCalled = false;

	std::function<void(int, bool)> receiver = [=, &receiverCalled](int p0, bool p1)
	{
		receiverCalled = true;

		ASSERT_TRUE(paramOne==p0); // First parameter should be as expected.
		ASSERT_TRUE(paramTwo==p1); // Second parameter should be as expected.
	};

	sg.connect(receiver, nullptr);

	lss::signal<void(int, bool)> sg2;
	sg2 = sg;
	sg2(paramOne, paramTwo);

	ASSERT_TRUE(receiverCalled); // Receiver2 should be called.

	receiverCalled = false;
	lss::signal<void(int, bool)> sg3 = sg;
	sg3(paramOne, paramTwo);

	ASSERT_TRUE(receiverCalled); // Receiver3 should be called.
}


TEST(lss_addConnectionInCallback, Base)
{
	TestA ta;
	connectionAddedInCallbackCalled = false;

	connection explicitConnectionA = ta.sigA.connect(&ta, &TestA::AddConnectionInCallbackA, &ta);
	receiveSigACount = 0;
	ta.sigA(12);
	
	ASSERT_FALSE(connectionAddedInCallbackCalled); // connectionAddedInCallbackCalled==false
	
	ASSERT_TRUE(receiveSigACount==1); 
	
	ta.sigA.disconnect(explicitConnectionA);
	receiveSigACount = 0;
	ta.sigA(22);
	
	ASSERT_TRUE(receiveSigACount==1);
	ASSERT_TRUE(connectionAddedInCallbackCalled);
}


TEST(lss_removeConnectionInCallback, Base)
{
	lss::signal<void()> sg;
	bool called = false;

	connection explicitConnection = sg.connect([&called]()
	{
		called = true;
	}, nullptr);
	
	sg();
	
	ASSERT_TRUE(called); // Connection not called"	

	sg.connect([&sg, &explicitConnection]()
	{
		sg.disconnect(explicitConnection);
	}, nullptr);
	sg();
	called = false;
	sg();
	
	ASSERT_FALSE(called); // Connection called, but not required it.
}


TEST(lss_addAndRemoveConnection, Base)
{
	lss::signal<void()> sg;
	bool called = false;

	connection explicitConnection = sg.connect([&called]()
	{
		called = true;
	}, nullptr);

	sg.disconnect(explicitConnection);
	sg();

	ASSERT_FALSE(called); // Connection called, but not required it.
	//-AssertHelper::VerifyValue(false, callled, "Connection called, but not required it.");
}


TEST(lss_destroyOwnerBeforeSignal, Base)
{
	TestB* pb = new TestB();

	lss::signal<void(int data)> sigA;
	sigA.connect(pb, &TestB::ReceiveSigA, pb);

	sigA(37);
	delete pb;
	
	ASSERT_TRUE(true); //TODO
}


TEST(lss_callSignalAfterDeleteOwner, Base)
{
	TestB* pb = new TestB();

	lss::signal<void(int data)> sigA;
	sigA.connect(pb, &TestB::ReceiveSigA, pb);

	sigA(37);

	delete pb;

	receiveSigACount = 0;
	sigA(42);
	
	ASSERT_TRUE(0==receiveSigACount); // Connection called, but not required it.
	//-AssertHelper::VerifyValue(0, receiveSigACount, "Connection called, but not required it.");
}


TEST(lss_callSignalAfterDeleteOwner2, Base)
{
	TestB* pb = new TestB();

	lss::signal<void(int data)> sigA;
	sigA.connect(pb, &TestB::ReceiveSigA, pb);

	delete pb;

	receiveSigACount = 0;
	sigA(42);
	ASSERT_TRUE(0==receiveSigACount); // Connection called, but not required it.
	//-AssertHelper::VerifyValue(0, receiveSigACount, "Connection called, but not required it.");
}


TEST(lss_deleteOwnerAndDisconnectAll, Base)
{
	TestB* pb = new TestB();

	lss::signal<void(int data)> sigA;
	sigA.connect(pb, &TestB::ReceiveSigA, pb);

	delete pb;
	sigA.disconnect_all();
	ASSERT_TRUE(true); //TODO
}

TEST(lss_disconnectAllInSignal, Base)
{
	TestA* pa = new TestA();
	TestB* pb = new TestB();
	pa->sigA.connect(pb, &TestB::ReceiveSigA, pb);

	receiveSigACount = 0;
	pa->sigA(37);
	ASSERT_TRUE(1==receiveSigACount); // Verify disconnect_all	
	//-AssertHelper::VerifyValue(1, receiveSigACount, "Verify disconnect_all");

	pa->sigA.connect([pa](int) {pa->sigA.disconnect_all(); }, pa);
	pa->sigA(37);

	receiveSigACount = 0;
	pa->sigA(37);
	ASSERT_TRUE(0==receiveSigACount); // Verify disconnect_all
	//-AssertHelper::VerifyValue(0, receiveSigACount, "Verify disconnect_all");

	delete pa;
	delete pb;
}


TEST(lss_recursiveSignalCall, Base)
{
	lss::signal<void()> sig;

	int recursive_count = 5;

	sig.connect([&sig, &recursive_count]()
	{
		recursive_count--;
		if (recursive_count)
			sig();
	}, nullptr);

	sig();
	
	ASSERT_TRUE(true); //TODO
}


TEST(lss_recursiveSignalAddDelete, Base)
{
	lss::signal<void(int)> sig;

	TestB* tb = new TestB;
	sig.connect(tb, &TestB::ReceiveSigA, tb);

	receiveSigACount = 0;
	sig(12);
	
	ASSERT_TRUE(1==receiveSigACount);
	//-AssertHelper::VerifyValue(1, receiveSigACount, "Verify 1");

	const int recursive_count = 7;
	int recursive_index = recursive_count;
	sig.connect([&sig, &recursive_index, &tb](int a)
	{
		recursive_index--;
		if (recursive_index)
			sig(a);
		else
		{
			delete tb;
			tb = nullptr;
		}
	}, nullptr);

	receiveSigACount = 0;
	sig(23);
	
	ASSERT_TRUE(recursive_count==receiveSigACount); // Verify recursive
	//-AssertHelper::VerifyValue(recursive_count, receiveSigACount, "Verify recursive");

	sig.disconnect_all();

	const int recursive_add = 3;
	recursive_index = recursive_count;
	sig.connect([&sig, &recursive_index, &tb](int a)
	{
		recursive_index--;

		if (recursive_index == recursive_add)
		{
			tb = new TestB;
			sig.connect(tb, &TestB::ReceiveSigA, tb);
		}

		if (recursive_index)
			sig(a);
	}, nullptr);

	receiveSigACount = 0;
	sig(33);

	ASSERT_TRUE(recursive_add==receiveSigACount); // Verify recursive
	//-AssertHelper::VerifyValue(recursive_add, receiveSigACount, "Verify recursive");
}


TEST(lss_connectEmptySignal, Base)
{
	std::function<void()> empty_fn_void;
	lss::signal<void()> sig_void;
	sig_void.connect(empty_fn_void, nullptr);
	sig_void();
	ASSERT_TRUE(true); //TODO

	std::function<int()> empty_fn_int;
	lss::signal<int()> sig_int;
	sig_int.connect(empty_fn_int, nullptr);
	sig_int();
	ASSERT_TRUE(true); //TODO
}


TEST(lss_connectionDisconnect, Base)
{
	int called = 0;
	lss::signal<void()> sig_void;
	connection cn = sig_void.connect([&called]() { called++; }, nullptr);

	sig_void();

	ASSERT_TRUE(1==called); // Called once
	//-AssertHelper::VerifyValue(1, called, "Called once");

	called = 0;
	cn.disconnect();
	sig_void();

	ASSERT_TRUE(0==called); // Dont call after disconnect
	//-AssertHelper::VerifyValue(0, called, "Dont call after disconnect");
}

TEST(lss_connectionDisconnectWithOwner, Base)
{
	int called = 0;
	lss::signal<void()> sig_void;
	slot owner;
	connection cn = sig_void.connect([&called]() { called++; }, &owner);

	sig_void();

	ASSERT_TRUE(1==called); // Called once
	//-AssertHelper::VerifyValue(1, called, "Called once");

	called = 0;
	cn.disconnect();
	sig_void();
	ASSERT_TRUE(0==called); // Dont call after disconnect
	//-AssertHelper::VerifyValue(0, called, "Dont call after disconnect");

	called = 0;
	cn.disconnect();
	sig_void();
	ASSERT_TRUE(0==called); // Dont call after disconnect
	//-AssertHelper::VerifyValue(0, called, "Dont call after disconnect");
}


TEST(lss_connectionDisconnectWithOwnerAfterOwnerDelete, Base)
{
	int called = 0;
	lss::signal<void()> sig_void;
	slot* owner = new slot();
	connection cn = sig_void.connect([&called]() { called++; }, owner);

	sig_void();

	ASSERT_TRUE(1==called); // Called once
	//-AssertHelper::VerifyValue(1, called, "Called once");

	delete owner;

	called = 0;
	sig_void();
	
	ASSERT_TRUE(0==called); // Dont call after disconnect
	//-AssertHelper::VerifyValue(0, called, "Dont call after disconnect");

	called = 0;
	cn.disconnect();
	sig_void();
	ASSERT_TRUE(0==called); // Dont call after disconnect
	//-AssertHelper::VerifyValue(0, called, "Dont call after disconnect");
}


TEST(lss_connectionDisconnectWithOwnerAfterSignalDelete, Base)
{
	int called = 0;
	lss::signal<void()>* sig_void = new lss::signal<void()>();
	slot* owner = new slot();
	connection cn = sig_void->connect([&called]() { called++; }, owner);

	(*sig_void)();
	
	ASSERT_TRUE(1==called); // Called once
	//-AssertHelper::VerifyValue(1, called, "Called once");

	delete sig_void;
	cn.disconnect();
	delete owner;
	
	ASSERT_TRUE(true); //TODO
}
