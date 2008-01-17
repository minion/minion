
# based on example from python docs.
import smtplib
# Import the email modules we'll need
from email.MIMEText import MIMEText

def mail(txt, subject="Minion testing error report"):
    # Create a text/plain message
    print "Attempting to send email."
    succeeded=False
    for i in range(5):
        try:
            msg = MIMEText(txt)
            
            # me == the sender's email address
            # you == the recipient's email address
            msg['Subject'] = subject
            msg['From'] = "pn@cs.st-and.ac.uk"
            msg['To'] = "pn@cs.st-and.ac.uk"
            
            # Send the message via our own SMTP server, but don't include the
            # envelope header.
            s = smtplib.SMTP("mail.cs.st-and.ac.uk")
            sendto=["pn@cs.st-and.ac.uk", "ipg@cs.st-and.ac.uk", "chris@bubblescope.net"]
            s.sendmail("pn@cs.st-and.ac.uk", sendto, msg.as_string())
            s.close()
            succeeded=True
            break
        except:
            print "An error occurred when attempting to send email."
            
    if succeeded:
        print "Succeeded in sending email."
    else:
        print "Failed to send email."
        
