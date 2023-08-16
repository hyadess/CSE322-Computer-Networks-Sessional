package Server;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.StringTokenizer;

public class UserInstance {
    private String userName;
    private DataInputStream in;
    private DataOutputStream out;
    boolean online;


    ArrayList<Notification> notifications=new ArrayList<>();

    UserInstance(DataInputStream in, DataOutputStream out)
    {
        this.in=in;
        this.out=out;
    }

    public void askUsername()
    {
        try {
            out.writeUTF("enter your username:");
            out.flush();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }

    public void inputUsername()
    {
        try {
            String p=in.readUTF();
            this.userName=p;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }


    public void writeText(String text)
    {
        try {
            out.writeUTF(text);
            out.flush();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }


    public void updateMessageList(ArrayList<Notification> notifications)
    {
        this.notifications=notifications;
    }



    public void createDirectory()
    {
        new File("files/"+userName+"/public").mkdirs();
        new File("files/"+userName+"/private").mkdirs();
    }

    public ArrayList<Notification> getNotifications() {
        return notifications;
    }

    String getUserName()
    {
        return userName;
    }
    boolean isOnline()
    {
        return online;
    }

    void makeOnline()
    {
        this.online=true;
    }

    void makeOffline()
    {
        this.online=false;
    }

    ArrayList<String> take_input()
    {
        try {
            String text = in.readUTF();
            StringTokenizer stringTokenizer = new StringTokenizer(text," ");
            ArrayList<String>tokens = new ArrayList<>();

            while (stringTokenizer.hasMoreTokens())
            {
                String token=stringTokenizer.nextToken();
                if(token.length()>0)
                {
                    tokens.add(token);
                }
            }
            return tokens;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }

    public void addNotification(String sender, String message)
    {
        Notification n=new Notification(sender,userName,message);
        notifications.add(n);

    }

    public DataInputStream getIn() {
        return in;
    }

    public DataOutputStream getOut() {
        return out;
    }
}
