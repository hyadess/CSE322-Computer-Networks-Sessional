package Server;

import java.awt.desktop.ScreenSleepEvent;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.Random;
import java.util.Scanner;

public class Server {

    private static long MAX_BUFFER_SIZE;
    private static long MAX_CHUNK_SIZE;
    private static long MIN_CHUNK_SIZE;
    private static long CUR_BUFFER_SIZE;

    private static  ArrayList<UserInstance> userList=new ArrayList<>();
    private static  ArrayList<UserInstance>requesterList=new ArrayList<>();
    private static ArrayList<FileInstance> fileList=new ArrayList<>();
    private static  int requestId=0;




    /// check the login info and take appropriate action for the new user...............
    private static boolean isAlreadyLoggedIn(UserInstance user)
    {
        boolean ans=false;
        for(UserInstance u:userList)
        {
            if(u.getUserName().equalsIgnoreCase(user.getUserName()) && u.isOnline()==true)
            {
                ans=true;
                break;
            }
        }
        return ans;
    }
    private static boolean loggedInBefore(UserInstance user)
    {
        boolean ans=false;
        for(UserInstance u:userList)
        {
            if(u.getUserName().equalsIgnoreCase(user.getUserName()) && u.isOnline()==false)
            {
                ans=true;
                break;
            }
        }
        return ans;
    }

    private static boolean check_for_user(UserInstance user) /// returns true if we need to stop the thread
    {
        user.askUsername();
        user.inputUsername();

        if(isAlreadyLoggedIn(user))  /// stop this thread.
        {
            user.writeText("You are already logged in. signing you out from this instance");
            return true;
        }
        else if(loggedInBefore(user))
        {
            user.writeText("WELCOME");
            UserInstance userToRemove=null;
            for(UserInstance u:userList)
            {
                if(u.getUserName().equalsIgnoreCase(user.getUserName()))
                {
                    user.updateMessageList(u.getNotifications());
                    userToRemove=u;
                    break;
                }
            }

            userList.remove(userToRemove);
            userList.add(user);
            user.makeOnline();
            return false;
        }
        else
        {
            user.writeText("WELCOME");
            userList.add(user);
            user.makeOnline();
            user.createDirectory();
            return  false;
        }

    }


    private static boolean check_for_valid_user(String userName)
    {
        boolean ans=false;
        for(UserInstance user:userList)
        {
            if(user.getUserName().equalsIgnoreCase(userName))
            {
                ans=true;
                break;
            }

        }
        return ans;
    }













    ///helper functions.................................

    private static UserInstance create_new_user(Socket socket)
    {
        try {
            DataOutputStream out = new DataOutputStream(socket.getOutputStream());
            DataInputStream in = new DataInputStream(socket.getInputStream());

            UserInstance newUser=new UserInstance(in,out);
            return newUser;

        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
    private static int generateChunkSize()
    {
        Random r = new Random();
        int result = r.nextInt((int) (MAX_CHUNK_SIZE-MIN_CHUNK_SIZE)) + (int)MIN_CHUNK_SIZE;
        return result;
    }
    private static int generateFileID(UserInstance user,String fileName, long fileSize,long chunkSize)
    {
        FileInstance newFile=new FileInstance(user.getUserName(),fileName,fileSize,fileList.size(),chunkSize);
        fileList.add(newFile);
        return fileList.size()-1;
    }

    private static UserInstance findUserWithRequestId(int requestId)
    {
        if(requestId<requesterList.size())
            return requesterList.get(requestId);
        else
            return null;
    }


    private static UserInstance findUserByName(String userName)
    {
        for(UserInstance user: userList)
        {
            if(user.getUserName().equalsIgnoreCase(userName))
                return user;
        }
        return null;
    }




    // functions for services.................

    ///service 1............... seeing user list
    private static String currentUserList()
    {
        String ans = "All Users : \n";

        for(UserInstance u:userList)
        {
            String status;
            if(u.isOnline()==true)
                status="ONLINE";
            else
                status="OFFLINE";
            ans += u.getUserName() + " ( " + status + " ) \n";
        }

        return ans;


    }




    ///service 2.................watching own files
    private static String publicFileList(String userName)
    {
        File path=new File("files/"+ userName+"/public");
        String[] content=path.list();
        String ans="";
        ans+="public files:\n";
        for(String it:content)
        {
            ans+=it+"\n";
        }
        return ans;


    }
    private static String privateFileList(String userName)
    {
        File path=new File("files/"+ userName+"/private");
        String[] content=path.list();
        String ans="";
        ans+="private files:\n";
        for(String it:content)
        {
            ans+=it+"\n";
        }
        return ans;


    }
    private static String ownFileList(String userName)
    {
        String ans=publicFileList(userName)+privateFileList(userName);
        return ans;
    }



    /// service 3................watching other's public file list


    private static String othersPublicFileList(String userName)
    {
        String ans="";
        for(UserInstance user: userList)
        {
            if(user.getUserName().equalsIgnoreCase(userName)==false)
            {
                File path=new File("files/"+ user.getUserName()+"/public");
                String[] content=path.list();
                ans+="public files of " + user.getUserName()+":\n";
                for(String it:content)
                {
                    ans+=it+"\n";
                }

            }
        }
        return ans;
    }





    /// service 4.................seeing unread message


    private static String messages(UserInstance user)
    {
        String ans="";
        ans+="Unread messages for "+user.getUserName()+":\n";
        ArrayList<Notification> message=user.getNotifications();
        for(Notification r:message)
        {
            if(r.isSeen()==false)
            {
                ans+=r.getMessage()+"\n";
                r.seeMessage();
            }
        }
        return ans;
    }




    ///service 5.................broadcast message


    private static void addRequest(String fileName, String description,UserInstance sender)
    {
        String broadcastMessage="user "+sender.getUserName()+" requested a file: \nfilename: "+fileName+
                "\ndescription: "+description +"\n request id: "+requestId;
        requesterList.add(sender);
        requestId++;
        for(UserInstance user:userList)
        {
            if(user.getUserName()!=sender.getUserName())
                user.addNotification(sender.getUserName(),broadcastMessage);
        }
    }



    /// service 6................uploading files.......


    private static boolean receiveFile(UserInstance user, FileInstance file,String fileType) throws IOException {

        int done = 0;
        FileOutputStream serverFile =new FileOutputStream("files/"+user.getUserName()+"/"+fileType+"/"+file.getFileName());

        try{
            long size = file.fileSize;     // read file size
            byte[] readAmount = new byte[(int) file.getChunkSize()];
            int chunk_no = 0;
            // extra
            CUR_BUFFER_SIZE += file.getChunkSize();

            // inside while loop, server reads chunks one by one and send acknowledgement....READ --> WRITE
            while (size > 0) {

                boolean ok;
                try {
                    ok = (done = user.getIn().read(readAmount, 0, (int) Math.min(readAmount.length, size))) != -1;
                }catch (SocketTimeoutException socketTimeoutException){
                    CUR_BUFFER_SIZE -= file.getChunkSize();
                    serverFile.close();
                    System.out.println("FileOutputStream Closed");
                    return false;
                }

                if(!ok) break;


                //System.out.println("Chunk #"+chunk_no);
                chunk_no++;


                serverFile.write(readAmount,0,done); // write to file..........

                size -= done;      // read upto file size

                //Thread.sleep(20000);
                // send ACK
                user.writeText("Acknowledged");   /// SERVER TO USER............



            }

            CUR_BUFFER_SIZE -=file.getChunkSize() ;
            serverFile.close();
            System.out.println("FileOutputStream Closed");

        }catch (Exception e)
        {
            System.out.println("Exception ... ");
            CUR_BUFFER_SIZE -= file.getChunkSize();
            serverFile.close();
            System.out.println("FileOutputStream Closed");
        }




        // after sending the last chunk, client send an completion message.....
        String msg = user.getIn().readUTF();    /// READ.....
        File uploadedFile = new File("files/"+user.getUserName()+"/"+fileType+"/"+file.getFileName());
        if(msg.equals("COMPLETE")){
            if(uploadedFile.length() != file.getFileSize())
            {
                System.out.println("File size mismatch");
                uploadedFile.delete();
                return false;
            }
        }
        else
        {
            uploadedFile.delete();
            return false;
        }
        System.out.println("file upload is successful");

        return true;
    }

    private static void sendFile(String fileName, String fileType,String fileOwner,UserInstance user,int chunkLength) throws IOException {

        File file = new File("files/"+fileOwner+"/"+fileType+"/"+fileName);
        FileInputStream fileInputStream = new FileInputStream(file);

        try{
            long fileLength = file.length();

            /// send user the file info and get his acknowledgement................
            user.writeText("fileInfo "+ fileLength +" "+fileName+" "+fileType+" "+chunkLength);
            System.out.println(user.getUserName());

            if(!user.take_input().get(0).equalsIgnoreCase("OK"))
                return;


            // break file into chunks
            int bytes = 0;
            byte[] buffer = new byte[chunkLength];
            int CHUNK = 0;


            while ((bytes=fileInputStream.read(buffer))!=-1){
                if(CHUNK % 10000 == 0) System.out.println("Chunk #"+CHUNK);
                CHUNK++;

                user.getOut().write(buffer,0,bytes);
                user.getOut().flush();
            }

            fileInputStream.close();
            System.out.println("file download is done. file stream is closed");

        }catch (Exception e)
        {
            fileInputStream.close();
            System.out.println("exception happened. file stream is closed");
        }
    }




    private static void  create_new_thread(Socket socket)
    {

        Thread worker = new Thread(new Runnable() {
            @Override
            public void run() {

                /// user create and check
                UserInstance newUser=create_new_user(socket);
                boolean y=check_for_user(newUser);
                if(y==true)
                {
                    Thread.currentThread().interrupt();
                    return;
                }

                /// start giving service....
                while (true)
                {
                    try{
                        ArrayList<String> clientMessage=newUser.take_input();
                        if(clientMessage.size()==0)
                        {
                            System.out.println("user "+newUser.getUserName()+": "+"null message");
                            continue;
                        }

                        System.out.println("user "+newUser.getUserName()+": "+clientMessage);
                        if(clientMessage.get(0).equalsIgnoreCase("show_users"))
                        {
                            newUser.writeText(currentUserList());

                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("show_my_files"))
                        {
                            newUser.writeText(ownFileList(newUser.getUserName()));

                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("show_others_files"))
                        {
                            newUser.writeText(othersPublicFileList(newUser.getUserName()));
                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("show_messages"))
                        {
                            newUser.writeText(messages(newUser));
                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("file_request"))
                        {
                            String filename=clientMessage.get(1);
                            String description=clientMessage.get(2);
                            addRequest(filename,description,newUser);
                            newUser.writeText("Your request is broadcast with request id "+(requestId-1));

                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("upload_info"))
                        {
                            String fileName=clientMessage.get(1);
                            long fileSize=Integer.parseInt(clientMessage.get(2));
                            //System.out.println(clientMessage.get(2));
                            if(CUR_BUFFER_SIZE+fileSize>MAX_BUFFER_SIZE)
                            {
                                newUser.writeText("impossible");
                            }
                            else
                            {
                                int chunkSize=generateChunkSize();
                                int fileID=generateFileID(newUser,fileName,fileSize,chunkSize);
                                String text="";
                                text+="possible ";
                                text+=fileName+" ";
                                text+=fileSize+" ";
                                text+=fileID+" ";
                                text+=chunkSize;
                                newUser.writeText(text);
                            }

                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("upload_on_request_info"))
                        {
                            int requestId=Integer.parseInt(clientMessage.get(1));
                            String fileName=clientMessage.get(2);
                            long fileSize=Integer.parseInt(clientMessage.get(3));
                            System.out.println(fileSize);
                            if(CUR_BUFFER_SIZE+fileSize>MAX_BUFFER_SIZE)
                            {
                                newUser.writeText("file upload is not possible. try again!!!");
                            }
                            else if(requestId>requesterList.size()-1 || requestId<0)
                            {
                                newUser.writeText("not a valid request id!!!");
                            }
                            else
                            {
                                int chunkSize=generateChunkSize();
                                int fileID=generateFileID(newUser,fileName,fileSize,chunkSize);
                                String text="";
                                text+="possible ";
                                text+=fileName+" ";
                                text+=requestId+" ";
                                text+=fileSize+" ";
                                text+=fileID+" ";
                                text+=chunkSize;
                                newUser.writeText(text);
                            }

                        }

                        else if(clientMessage.get(0).equalsIgnoreCase("upload_file"))
                        {
                            int fileId=Integer.parseInt(clientMessage.get(1));
                            String fileType=clientMessage.get(2);
                            if(fileId<0 || fileId>fileList.size()-1 ||
                                    (!fileType.equalsIgnoreCase("public") && !fileType.equalsIgnoreCase("private")))
                            {
                                newUser.writeText("inconsistent info");
                            }
                            else
                            {
                                newUser.writeText("OK_SEND_FILE");
                                try {
                                    socket.setSoTimeout(30000);
                                    boolean success=receiveFile(newUser,fileList.get(fileId),fileType);
                                    socket.setSoTimeout(0);
                                    if(success)
                                        newUser.writeText("ACK");
                                    else
                                        newUser.writeText("NO_ACK");
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                            }

                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("upload_file_on_request"))
                        {
                            int fileId=Integer.parseInt(clientMessage.get(1));
                            int requestId=Integer.parseInt(clientMessage.get(2));
                            if(fileId<0 || fileId>fileList.size()-1 ||
                                    requestId<0 || requestId>requesterList.size()-1)
                            {
                                newUser.writeText("inconsistent info");
                            }
                            else
                            {
                                newUser.writeText("OK_SEND_FILE");
                                try {
                                    socket.setSoTimeout(30000);
                                    boolean success=receiveFile(newUser,fileList.get(fileId),"public");
                                    socket.setSoTimeout(0);
                                    if(success) {
                                        newUser.writeText("ACK");
                                        UserInstance requester=findUserWithRequestId(requestId);
                                        requester.addNotification(newUser.getUserName(),
                                                "your request file with id "+requestId+" is uploaded by "+newUser.getUserName());

                                    }
                                    else
                                        newUser.writeText("NO_ACK");
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                            }



                        }

                        else if(clientMessage.get(0).equalsIgnoreCase("download_my_file"))
                        {
                            String fileType=clientMessage.get(1);
                            String fileName=clientMessage.get(2);
                            try {
                                sendFile(fileName,fileType,newUser.getUserName(),newUser, (int) MAX_CHUNK_SIZE);
                            } catch (IOException e) {
                                e.printStackTrace();
                            }

                        }

                        else if(clientMessage.get(0).equalsIgnoreCase("download_others_file"))
                        {
                            String userName=clientMessage.get(1);
                            String fileName=clientMessage.get(2);
                            if(!check_for_valid_user(userName))
                            {
                                newUser.writeText("not_a_user");
                            }
                            else
                            {
                                UserInstance user=findUserByName(userName);
                                try {
                                    sendFile(fileName,"public",user.getUserName(),newUser, (int) MAX_CHUNK_SIZE);
                                } catch (IOException e) {
                                    e.printStackTrace();
                                }

                            }


                        }

                        else if(clientMessage.get(0).equalsIgnoreCase("TIMEOUT"))
                        {
                            String fileType = clientMessage.get(1);
                            String fileName = clientMessage.get(2);

                            File file = new File("files/"+newUser.getUserName()+"/"+fileType+"/"+fileName);
                            System.out.println(file.delete());
                            newUser.writeText("File Deleted");
                        }
                        else if(clientMessage.get(0).equalsIgnoreCase("EXIT"))
                        {
                            System.out.println(newUser.getUserName()+" is offline");
                            newUser.makeOffline();
                            Thread.currentThread().interrupt(); // preserve the message
                            return;
                        }
                        else
                        {
                            newUser.writeText("unknown command");
                            continue;
                        }

                    }catch (Exception e)
                    {
                        System.out.println(newUser.getUserName()+" is offline");
                        newUser.makeOffline();
                        Thread.currentThread().interrupt(); // preserve the message
                        return;
                    }



                }
            }
        });
        worker.start();

    }






    public static void main(String[] args) throws IOException, ClassNotFoundException {

        Scanner sc=new Scanner(System.in);
        System.out.println("enter the MAX_BUFFER_SIZE:");
        MAX_BUFFER_SIZE=sc.nextLong();
        System.out.println("enter the MAX_CHUNK_SIZE:");
        MAX_CHUNK_SIZE=sc.nextLong();
        System.out.println("enter the MIN_CHUNK_SIZE");
        MIN_CHUNK_SIZE=sc.nextLong();
        CUR_BUFFER_SIZE=0;
        ServerSocket welcomeSocket = new ServerSocket(6666);

        while(true) {
            System.out.println("Waiting for connection...");
            Socket socket = welcomeSocket.accept();
            System.out.println("Connection established");


            // open thread and continue for next user......
            create_new_thread(socket);


        }

    }
}