package Server;

public class FileInstance {
    String userName;
    String fileName;
    long fileSize;
    int fileID;
    long chunkSize;


    public FileInstance(String userName, String fileName, long fileSize, int fileID, long chunkSize)
    {
        this.userName=userName;
        this.fileName=fileName;
        this.fileSize=fileSize;
        this.fileID=fileID;
        this.chunkSize=chunkSize;
    }


    public long getChunkSize() {
        return chunkSize;
    }

    public int getFileID() {
        return fileID;
    }

    public long getFileSize() {
        return fileSize;
    }

    public String getFileName() {
        return fileName;
    }

    public String getUserName() {
        return userName;
    }
}
