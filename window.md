# rfc793, section 3.7, Data Communication

* segments may be lost due to 
  * errors (checksum test failure), 
  * network congestion
* TCP uses **retransmission** (after a timeout) to ensure delivery of every segment. 
  Duplicate segments may arrive due to network or TCP retransmission.
* **SND.NXT**: The sender keeps track of the next sequence number to use. When the sender creates a segment and transmits it the sender **advances
  SND.NXT**.
* **SND.UNA**: The sender of data keeps track of the **oldest unacknowledged sequence number**. When the data sender receives an
  acknowledgment it **advances SND.UNA**.
* **RCV.NXT**: The receiver keeps track of the next sequence number to expect.  When the receiver accepts a segment it **advances RCV.NXT** and
  **sends an acknowledgment**.
* **Note that once in the ESTABLISHED state all  segments must carry current acknowledgment information.** 


##Retransmission Timeout

Because of the variability of the networks that compose an
inter-network system and the wide range of uses of TCP connections the
retransmission timeout must be dynamically determined.  One procedure
for determining a retransmission time out is given here as an
illustration.

    An Example Retransmission Timeout Procedure

      Measure the elapsed time between sending a data octet with a
      particular sequence number and receiving an acknowledgment that
      covers that sequence number (segments sent do not have to match
      segments received).  This measured elapsed time is the Round Trip
      Time (RTT).  Next compute a Smoothed Round Trip Time (SRTT) as:

        SRTT = ( ALPHA * SRTT ) + ((1-ALPHA) * RTT)

      and based on this, compute the retransmission timeout (RTO) as:

        RTO = min[UBOUND,max[LBOUND,(BETA*SRTT)]]

      where UBOUND is an upper bound on the timeout (e.g., 1 minute),
      LBOUND is a lower bound on the timeout (e.g., 1 second), ALPHA is
      a smoothing factor (e.g., .8 to .9), and BETA is a delay variance
      factor (e.g., 1.3 to 2.0).

##The Communication of Urgent Information

The objective of the TCP urgent mechanism is to allow the sending user
to stimulate the receiving user to accept some urgent data and to
permit the receiving TCP to indicate to the receiving user when all
the currently known urgent data has been received by the user.

This mechanism permits a point in the data stream to be designated as
the end of urgent information.  Whenever this point is in advance of
the receive sequence number (RCV.NXT) at the receiving TCP, that TCP
must tell the user to go into "urgent mode"; when the receive sequence
number catches up to the urgent pointer, the TCP must tell user to go
into "normal mode".  If the urgent pointer is updated while the user
is in "urgent mode", the update will be invisible to the user.

The method employs a urgent field which is carried in all segments
transmitted.  The URG control flag indicates that the urgent field is
meaningful and must be added to the segment sequence number to yield
the urgent pointer.  The absence of this flag indicates that there is
no urgent data outstanding.

To send an urgent indication the user must also send at least one data
octet.  If the sending user also indicates a push, timely delivery of
the urgent information to the destination process is enhanced.

##Managing the Window

The window sent in each segment indicates the range of sequence
numbers the sender of the window (the data receiver) is currently
prepared to accept.  There is an assumption that this is related to
the currently available data buffer space available for this
connection.

Indicating a large window encourages transmissions.  If more data
arrives than can be accepted, it will be discarded.  This will result
in excessive retransmissions, adding unnecessarily to the load on the
network and the TCPs.  Indicating a small window may restrict the
transmission of data to the point of introducing a round trip delay
between each new segment transmitted.

The mechanisms provided allow a TCP to advertise a large window and to
subsequently advertise a much smaller window without having accepted
that much data.  This, so-called "shrinking the window," is strongly
discouraged.  The robustness principle dictates that TCPs will not
shrink the window themselves, but will be prepared for such behavior
on the part of other TCPs.

The sending TCP must be prepared to accept from the user and send at
least one octet of new data even if the send window is zero.  The
sending TCP must regularly retransmit to the receiving TCP even when
the window is zero.  Two minutes is recommended for the retransmission
interval when the window is zero.  This retransmission is essential to
guarantee that when either TCP has a zero window the re-opening of the
window will be reliably reported to the other.

When the receiving TCP has a zero window and a segment arrives it must
still send an acknowledgment showing its next expected sequence number
and current window (zero).

The sending TCP packages the data to be transmitted into segments
which fit the current window, and may repackage segments on the
retransmission queue.  Such repackaging is not required, but may be
helpful.

In a connection with a one-way data flow, the window information will
be carried in acknowledgment segments that all have the same sequence
number so there will be no way to reorder them if they arrive out of
order.  This is not a serious problem, but it will allow the window
information to be on occasion temporarily based on old reports from
the data receiver.  A refinement to avoid this problem is to act on
the window information from segments that carry the highest
acknowledgment number (that is segments with acknowledgment number
equal or greater than the highest previously received).

The window management procedure has significant influence on the
communication performance.  The following comments are suggestions to
implementers.

##Window Management Suggestions

  Allocating a very small window causes data to be transmitted in
  many small segments when better performance is achieved using
  fewer large segments.

  One suggestion for avoiding small windows is for the receiver to
  defer updating a window until the additional allocation is at
  least X percent of the maximum allocation possible for the
  connection (where X might be 20 to 40).

  Another suggestion is for the sender to avoid sending small
  segments by waiting until the window is large enough before
  sending data.  If the user signals a push function then the
  data must be sent even if it is a small segment.

  Note that the acknowledgments should not be delayed or unnecessary
  retransmissions will result.  One strategy would be to send an
  acknowledgment when a small segment arrives (without updating the
  window information), and then to send another acknowledgment with
  new window information when the window is larger.

  The segment sent to probe a zero window may also begin a break up
  of transmitted data into smaller and smaller segments.  If a
  segment containing a single data octet sent to probe a zero window
  is accepted, it consumes one octet of the window now available.
  If the sending TCP simply sends as much as it can whenever the
  window is nonzero, the transmitted data will be broken into
  alternating big and small segments.  As time goes on, occasional
  pauses in the receiver making window allocation available will
  result in breaking the big segments into a small and not quite so
  big pair. And after a while the data transmission will be in
  mostly small segments.

  The suggestion here is that the TCP implementations need to
  actively attempt to combine small window allocations into larger
  windows, since the mechanisms for managing the window tend to lead
  to many small windows in the simplest minded implementations.