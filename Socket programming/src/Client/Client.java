package Client;

import Server.FileInstance;

import java.io.*;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Scanner;
import java.util.StringTokenizer;

public class Client {

    private static String userName;
    private static ArrayList<FileInstance> fileList = new ArrayList<>();


    private static FileInstance findFileByFileId(int fileId) {
        for (FileInstance f : fileList) {
            if (f.getFileID() == fileId)
                return f;
        }
        return null;
    }

    private static void interfaceOutput() {
        System.out.println("Commands for services");
        System.out.println("to see full user list: show_users");
        System.out.println("to see your public and private file: show_my_files");
        System.out.println("to see others public file: show_others_files");
        System.out.println("to see unread messages: show_messages");
        System.out.println("to request for a file: file_request <file_name> <description> ");
        System.out.println("to inform server about a file: upload_info");
        System.out.println("to inform server about a requested file: upload_on_request_info");
        System.out.println("to upload a file: upload_file <fileId> <fileType>");
        System.out.println("to upload a requested file: upload_file_on_request <fileId> <requestId>");
        System.out.println("to download own file: download_my_file <fileType> <fileName>");
        System.out.println("to download others file: download_others_file <userName> <fileName>");
        System.out.println("EXIT");


    }

    private static ArrayList<String> dissect(String s) {

        String text = s;
        StringTokenizer stringTokenizer = new StringTokenizer(text, " ");
        ArrayList<String> tokens = new ArrayList<>();

        while (stringTokenizer.hasMoreTokens()) {
            String token = stringTokenizer.nextToken();
            if (token.length() > 0) {
                tokens.add(token);
            }
        }
        return tokens;


    }


    private static void sendFile(FileInstance file, DataInputStream in, DataOutputStream out, String fileType) throws IOException {

        File clientFile = new File("clientFile/" + userName + "/" + file.getFileName());
        FileInputStream fileInputStream = new FileInputStream(clientFile);

        long fileLength = file.getFileSize();

        // break file into chunks
        int byteCount = 0;
        byte[] buffer = new byte[(int) file.getChunkSize()];
        int chunk_no = 0;
        while ((byteCount = fileInputStream.read(buffer)) != -1) {

//          if(CHUNK % 10000 == 0) System.out.println("Chunk #"+CHUNK);
            chunk_no++;

            out.write(buffer, 0, byteCount);
            out.flush();


            try {
                // ACK
                String msg = in.readUTF();
                if (!msg.equals("Acknowledged")) {
                    System.out.println("Did not receive acknowledgment...");
                    break;
                }

            } catch (SocketTimeoutException socketTimeoutException) {
                System.out.println("TIMEOUT");
                out.writeUTF("TIMEOUT " + fileType + " " + file.getFileName());
                out.flush();
                fileInputStream.close();
                return;
            }
        }

        fileInputStream.close();

        // send confirmation
        out.writeUTF("COMPLETE");
        out.flush();

        String msg = in.readUTF();
        if (msg.equals("ACK")) System.out.println("File Upload Completed");
        else System.out.println("File Upload Failed");

    }


    private static void receiveFile(String fileName, String fileType,long filesize,DataInputStream dataInputStream,int chunkLength) throws IOException {

        int bytes = 0;

        FileOutputStream fileOutputStream = new FileOutputStream("clientFile/"+userName+"_downloaded_"+fileName);

        long size = filesize;     // read file size
        byte[] buffer = new byte[chunkLength];
        int chunkNo = 0;
        while (size > 0 && (bytes = dataInputStream.read(buffer, 0, (int) Math.min(buffer.length, size))) != -1) {
//            System.out.println("Chunk #"+chunkNo);
            chunkNo++;
            fileOutputStream.write(buffer,0,bytes);
            size -= bytes;      // read upto file size

        }
        fileOutputStream.close();
        System.out.println("download completed. file stream is closed");
    }


    public static void main(String[] args) throws IOException, ClassNotFoundException {
        Socket socket = new Socket("localhost", 6666);
        System.out.println("Connection established");
        System.out.println("Remote port: " + socket.getPort());
        System.out.println("Local port: " + socket.getLocalPort());

        // buffers
        DataOutputStream out = new DataOutputStream(socket.getOutputStream());
        DataInputStream in = new DataInputStream(socket.getInputStream());


        //
        Scanner sc = new Scanner(System.in);

        String msg = in.readUTF();  // server asking username
        System.out.println(msg);
        userName = sc.nextLine();
        out.writeUTF(userName); // we are giving our username
        out.flush();
        String conf = in.readUTF(); // if conf is WELCOME, it means our connection is done

        if (conf.equalsIgnoreCase("WELCOME")) {
            while (true) {
                interfaceOutput();
                String p = sc.nextLine();
                if (p.equalsIgnoreCase("upload_info")) {
                    System.out.println("enter your file name:\n");
                    String f = sc.nextLine();
                    File clientFile = new File("clientFile/" + userName + "/" + f);
                    int filesize = (int) clientFile.length();
                    p = p + " " + f + " " + filesize;

                }
                if (p.equalsIgnoreCase("upload_on_request_info")){
                    System.out.println("enter your file name:\n");
                    String f = sc.nextLine();
                    System.out.println("enter the request id");
                    int k=sc.nextInt();
                    sc.nextLine();
                    File clientFile = new File("clientFile/" + userName + "/" + f);
                    int filesize = (int) clientFile.length();
                    p = p + " " +k+" "+ f + " " + filesize;

                }

                out.writeUTF(p);
                out.flush();


                String i = in.readUTF();
                System.out.println("server sent:" +i);
                ArrayList<String> input = dissect(p);
                ArrayList<String> output = dissect(i);

                if (input.get(0).equalsIgnoreCase("upload_info")) {
                    System.out.println("if it is possible, server will send a message in this format:");
                    System.out.println("possible <fileName> <fileSize> <fileId> <chunkSize>");
                    if (output.get(0).equalsIgnoreCase("possible")) {
                        FileInstance file = new FileInstance(userName, output.get(1), Integer.parseInt(output.get(2)),
                                Integer.parseInt(output.get(3)), Integer.parseInt(output.get(4)));
                        fileList.add(file);
                    }

                }
                else if (input.get(0).equalsIgnoreCase("upload_on_request_info")) {
                    System.out.println("if it is possible, server will send a message in this format:");
                    System.out.println("possible <fileName> <requestId> <fileSize> <fileId> <chunkSize>");
                    if (output.get(0).equalsIgnoreCase("possible")) {
                        FileInstance file = new FileInstance(userName, output.get(1), Integer.parseInt(output.get(2)),
                                Integer.parseInt(output.get(4)), Integer.parseInt(output.get(5)));
                        fileList.add(file);
                    }
                }

                else if (input.get(0).equalsIgnoreCase("upload_file")) {
                    int fileId = Integer.parseInt(input.get(1));
                    String fileType = input.get(2);
                    FileInstance file = findFileByFileId(fileId);
                    if (output.get(0).equalsIgnoreCase("OK_SEND_FILE")) {
                        socket.setSoTimeout(30000);
                        sendFile(file, in, out, fileType);
                        socket.setSoTimeout(0);
                    }
                }
                else if (input.get(0).equalsIgnoreCase("upload_file_on_request")) {
                    int fileId = Integer.parseInt(input.get(1));
                    FileInstance file = findFileByFileId(fileId);
                    //System.out.println(output);
                    if (output.get(0).equalsIgnoreCase("OK_SEND_FILE")) {
                        socket.setSoTimeout(30000);
                        sendFile(file, in, out, "public");
                        socket.setSoTimeout(0);
                    }
                }

                else if(input.get(0).equalsIgnoreCase("download_my_file")
                || input.get(0).equalsIgnoreCase("download_others_file"))
                {



                    ArrayList<String> infoList=output;
                    long fileLength= Integer.parseInt(infoList.get(1));
                    String fileName=infoList.get(2);
                    String fileType=infoList.get(3);
                    int chunkLength= Integer.parseInt(infoList.get(4));
                    out.writeUTF("OK");
                    out.flush();
                    receiveFile(fileName,fileType,fileLength,in,chunkLength);




                }

                else if(input.get(0).equalsIgnoreCase("EXIT"))
                {
                    break;
                }





            }
        }

    }
}
