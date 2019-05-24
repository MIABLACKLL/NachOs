// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#define INT_MAX 2147483647
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    
    int type = kernel->machine->ReadRegister(2);
    int result;
    char *argv[5];
    char buf[80];
    int iterator;
    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
    //cerr<<SyscallException<<" "<<IllegalInstrException<<" "<<OverflowException<<" "<<AddressErrorException<<" "<<PageFaultException<<"\n";
    //cerr<<which<<" "<<type<<"\n";
    switch (which) {
    case PageFaultException:
	//cout<<"test!";
	{
	TranslationEntry* globalPageTable = kernel->machine->globalPageTable;
	int leastUsedPage=-1;
	int timetamp=INT_MAX;
	for(int i=0;i<NumPhysPages;i++)
	{
		if(globalPageTable[i].recUsed<timetamp)
		{
		  leastUsedPage=i;
		  timetamp=globalPageTable[i].recUsed;
	          //cout<<globalPageTable[i].recUsed<<endl;
		}
	}
	globalPageTable[leastUsedPage].valid=TRUE;
	globalPageTable[leastUsedPage].use=FALSE;
	globalPageTable[leastUsedPage].recUsed=clock();
	cout<<"Catch PageFaultException,change page "<<leastUsedPage<<endl;
	//cout<<"test!";
	return;
	break;
	}
    case SyscallException:
      switch(type) {
      //case SC_Exit:
	//exit(0);
	//return;
      case SC_Halt:
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();

	ASSERTNOTREACHED();
	break;

      case SC_Add:
	DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	cerr<<"result is "<<result<<"\n";
	/* Modify return point */
	{
	  /* set previous programm counter (debugging only)*/
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	  /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	  /* set next programm counter for brach execution */
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}

	return;
	
	ASSERTNOTREACHED();

	break;


      case SC_Write:
	//int result;
	//cerr<<"SC_Write test begin!\n";
	//cerr<<kernel->machine->ReadRegister(4)<<" "<<kernel->machine->ReadRegister(5)<<" "<<kernel->machine->ReadRegister(6)<<"\n";
	result=SysWrite((int)kernel->machine->ReadRegister(4),(int)kernel->machine->ReadRegister(5),(int)kernel->machine->ReadRegister(6));
	//cerr<<"SC_Write test now!\n";
	kernel->machine->WriteRegister(2,(int)result);
	{
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);  
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}
	//cerr<<"SC_Write test ok!\n";
	return;
	
	ASSERTNOTREACHED();

	break;


      case SC_Read:
	
	//int result;
	//cerr<<"SC_Read test begin!\n";
	for(int i=1;i<=4;++i)
		argv[i]=(unsigned int)kernel->machine->ReadRegister(i+3);
	DEBUG(dbgSys,"SC_Read with parameters are:"<<argv[1]<<"\t"<<argv[2]<<"\t"<<argv[3]<<"\n");
	do{
		int size=(int)argv[2];
		int addr=(int)argv[1];
		//char *interator=argv[1];
		result=SysRead(buf,size,(OpenFileId)argv[3]);
		//cerr<<buf[0]<<" "<<size<<" "<<addr<<"\n";
		for(iterator=0;iterator!=size;++iterator)
			kernel->machine->WriteMem(addr++,1,buf[iterator]);
	}while(false);
	//cerr<<"SC_Read test now!\n";
	kernel->machine->WriteRegister(2,(int)result);
	{
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);  
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}
	return;
	//cerr<<"SC_Read test ok!\n";
	ASSERTNOTREACHED();
	break;


       case SC_Exec:
	//char *argv[5];
	//cerr<<"SC_Exec test begin!\n";
	argv[1]=(unsigned int)kernel->machine->ReadRegister(4);
	DEBUG(dbgSys,"SC_Exec with parameters are:"<<argv[1]<<"\n");
	iterator=0;
	do{
		kernel->machine->ReadMem((int)argv[1]+iterator,1,(int*)&buf[iterator]);
		//cerr<<buf[iterator]<<"\n";
		++iterator;
	}while(iterator<80&&buf[iterator-1]!='\0');
	//cerr<<"SC_Exec test now!\n";
	result=SysExec(buf);
	kernel->machine->WriteRegister(2,(int)result);
	{
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);  
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}
	//cerr<<"SC_Exec test ok!\n";
	return;
	ASSERTNOTREACHED();
	break;


      case SC_Join:
	//char *argv[5];
	argv[1]=(unsigned int)kernel->machine->ReadRegister(4);
	DEBUG(dbgSys,"SC_Exec with parameters are:"<<argv[1]<<"\n");
	result=SysJoin((SpaceId)argv[1]);
	kernel->machine->WriteRegister(2,(int)result);
	{
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);  
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}
	return;
	ASSERTNOTREACHED();
	break;

      default:
	cerr << "Unexpected system call " << type << "\n";
	break;
      }

    default:
      cerr << "Unexpected user mode exception " << (int)which << "\n";
      break;
    }
    ASSERTNOTREACHED();
}
