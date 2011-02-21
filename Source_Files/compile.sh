#!/bin/bash

TARGETPBP="alephone.elf";
PRNAME="AlephOne PSP";
MAKE1="Makefile";
MAKE2="Makefile2";
FIXUPI="psp-fixup-imports";
LOGFLE="compile.log";
CPDIRS="__SCE__alephone %__SCE__alephone";
TARGETDIRS=$CPDIRS;
BINAME="EBOOT.PBP";

PSPPATH="/media/PSP_";
PSPBASEPATH="/PSP/GAME150";

echo "========================================="
echo "$PRNAME compilation script..."
echo "========================================="

if [ -z $1 ]
then
 echo "Usage: $0 compile | skip | clean"
 exit -1
fi

	if [ $1 = "compile" ]
	then

		for ddirs in $TARGETDIRS;
		 do 
		  if ls $ddirs/$BINAME &> /dev/null;
		   then 
		     echo "Removing previous PBP file \"$ddirs/$BINAME\"";
		     rm $ddirs/$BINAME;
		  fi
		done;

		if ls $TARGETPBP &> /dev/null;
		then
		 echo "Removing target file \"$TARGETPBP\"";
		 rm $TARGETPBP;
		fi

		if ! ls $MAKE1 &> /dev/null;
		then
		 echo "Makefile: \"$MAKE1\" is not present!";
		 exit -1;
		fi

		if ! ls $MAKE2 &> /dev/null;
		then
		 echo "Makefile: \"$MAKE2\" is not present!";
		 exit -2;
		fi

		echo "Compiling $PRNAME (stage 1)..";
		if ! make -f $MAKE1 &> $LOGFLE;
		then
		 echo -e "\E[31mError compiling!\E[30m";
		 echo -e "\E[31m`grep "error" $LOGFLE`\E[30m";
		 echo -e "\E[31mCheck \"$LOGFLE\" for further infos\E[30m";
		 exit -3;
		fi 

		echo "Fixing up ELF imports (stage 2)..";
		if ! $FIXUPI $TARGETPBP >> $LOGFLE 2>&1;
		then
		 echo -e "\E[31mError fixing up imports!\E[30m";
		 echo -e "\E[31m`grep "error" $LOGFLE`\E[30m";
		 echo -e "\E[31mCheck \"$LOGFLE\" for further infos\E[30m"
		fi

		echo "Linking & Generating PBP $PRNAME (stage 3)..";
		if ! make -f $MAKE2 SCEkxploit>> $LOGFLE 2>&1;
		then
		 echo -e "\E[31mError Linking!\E[30m"
		 echo -e "\E[31m`grep "error" $LOGFLE`\E[30m";
		 echo -e "\E[31mCheck \"$LOGFLE\" for further infos\E[30m"
		 exit -3;
		fi

		for ddirs in $TARGETDIRS;
		 do 
		  if ! ls $ddirs &> /dev/null;
		   then echo "Compilation completed succesfully!";
		  fi
		done;

	elif [ $1 = "clean" ]
	then

		if ! ls $MAKE1 &> /dev/null;
		then
		 echo "Makefile: \"$MAKE1\" is not present!";
		 exit -1;
		fi

		echo "Cleaning...";
		make -f $MAKE1 clean &> /dev/null;
		exit 0;

	fi;

echo "Checking for a connected Sony PSP";
if ! ls $PSPPATH &> /dev/null;
 then 
   echo "No Sony PSP found";
   echo "Skipping..";
   echo "All done OK!";
   exit;
fi

echo -e "\E[34mSony PSP found on $PSPPATH\E[30m";
if ! ls $PSPPATH$PSPBASEPATH &> /dev/null;
 then
   echo "Directory $PSPBASEPATH is does not exist on the PSP!";
   echo "Skipping..";
   echo "All done OK!";
fi;

for ddirs in $TARGETDIRS;
 do 
  if ls $PSPPATH$PSPBASEPATH/$ddirs/$BINAME &> /dev/null;
   then 
     echo "Removing previous PBP file on PSP \"$PSPPATH$PSPBASEPATH/$ddirs/$BINAME\"";
     rm $PSPPATH$PSPBASEPATH/$ddirs/$BINAME;
  fi
done;

echo "Copying new binary files on the PSP...";
for cdirs in $CPDIRS;
 do
  cp $cdirs/$BINAME $PSPPATH$PSPBASEPATH/$cdirs/$BINAME
done;

echo "All done OK!";

exit;
