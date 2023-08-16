package Server;

public class Notification {
    String sender;
    String receiver;
    String message;
    boolean seen;

    public Notification(String sender,String receiver,String message)
    {
        this.sender=sender;
        this.receiver=receiver;
        this.message=message;
        this.seen=false;
    }

    public String getMessage() {
        return message;
    }

    public String getReceiver() {
        return receiver;
    }

    public String getSender() {
        return sender;
    }


    public boolean isSeen() {
        return seen;
    }


    public void setMessage(String message) {
        this.message = message;
    }

    public void setReceiver(String receiver) {
        this.receiver = receiver;
    }

    public void setSender(String sender) {
        this.sender = sender;
    }
    public void seeMessage()
    {
        this.seen=true;
    }
}
