nPOP Ver 1.2.8
--

[Introduction]

This software allows you to access e-mail on a POP3 mail server on your PC. It provides the
function of common remote e-mail access.

The mail displayed by nPOP is only the mail currently on the server. Therefore, if mail is
deleted from the server it disappears from the list, even if it was displayed there before.

When you connect using nPOP, the mail is listed and you can see the header only.  From that
list you can select e-mail you want to receive on your PC. This is more efficient than
downloading all the e-mail on your PC.

It can also be used simply to check for the arrival of new e-mail, or to manage the mail
held on a server.

[Function]

- Receive E-Mail (POP3 and APOP)
- Send Mail (SMTP and SMTP-AUTH and POP before SMTP)
- User specified ports for POP3 and SMTP
- Multi-account support (all accounts can be checked in turn)
- Automatic check for new mail
- Mail can be saved in the Savebox
- A thread display of the mail list
- Support for attachments when sending and receiving e-mail (RFC 2231)
- Filters for received e-mail
- Simple address book
- TLS/SSL/STARTTLS

[Environment]

- Windows 7, 8, 8.1, 10, 11

- nPOP for iPhone
    https://apps.apple.com/app/npop-for-iphone/id368477936

[Update History]

・Ver 1.2.7 -> Ver 1.2.8
    ・Security fixes
        ・Fixed memory corruption when parsing malformed mail and server responses
        ・Stricter validation of attachment save paths and file names
        ・SMTP sending no longer continues in plain text when STARTTLS is unavailable
        ・The attach and mailbox parameters of mailto: are now accepted only from the command line
        ・On an unencrypted connection, SMTP authentication that sends the password as plain text (LOGIN/PLAIN) is no longer selected automatically
        ・The APOP timestamp is now validated
    ・Fixed long passwords being lost when saved

・Ver 1.2.6 -> Ver 1.2.7
    ・Fixed an issue where reception would not proceed after an SSL connection was established
    ・Added support for TLS 1.3

--

Special thanks:
  Manabu & Naomi Anzai
  Chris De Herrera
  Marlof Bregonje
  Takehiko Hanada
  Antoine
  Greg Chapman
  Fukuda

--

Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
	https://nakka.com/

10/27/2026
