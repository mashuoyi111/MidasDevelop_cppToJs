      SUBROUTINE PROCESS_EVENT (HBUF, HREQ, HEADER, EVENT)
	INTEGER*4 HBUF, HREQ, HEADER(4), EVENT(*)
        WRITE (*,*) HEADER(2)
	END

      PROGRAM TEST
      INCLUDE 'midas.inc'
      INTEGER*4 STATUS,REQUEST_ID
	INTEGER*4 HBUFEVENT

	STATUS = CM_CONNECT_EXPERIMENT('','sample','Fortran Analyzer')
	IF (STATUS .NE. CM_SUCCESS) STOP
	
	CALL BM_OPEN_BUFFER('SYSTEM', EVENT_BUFFER_SIZE, HBUFEVENT)
	CALL BM_REQUEST_EVENT(HBUFEVENT, 1, TRIGGER_ALL, GET_ALL, 
     +                      REQUEST_ID)

      DO WHILE (STATUS .NE. RPC_SHUTDOWN .AND. STATUS .NE. SS_ABORT) 
        STATUS = CM_YIELD(1000)
	END DO

      CALL CM_DISCONNECT_EXPERIMENT()

	END