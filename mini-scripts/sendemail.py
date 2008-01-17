
# based on example from python docs.
import smtplib
# Import the email modules we'll need
from email.MIMEText import MIMEText

def mail(txt):
    # Create a text/plain message
    try:
        msg = MIMEText(txt)
        
        # me == the sender's email address
        # you == the recipient's email address
        msg['Subject'] = 'Minion testing error report'
        msg['From'] = "pn@cs.st-and.ac.uk"
        msg['To'] = "pn@cs.st-and.ac.uk"
        
        # Send the message via our own SMTP server, but don't include the
        # envelope header.
        s = smtplib.SMTP("mail.cs.st-and.ac.uk")
        s.sendmail("pn@cs.st-and.ac.uk", ["pn@cs.st-and.ac.uk"], msg.as_string())
        s.close()
    except:
        # recursive call.
        # This is dangerous, but I'm not too bothered really.
        print "An error occurred when attempting to send email. Retrying."
        mail(txt)
