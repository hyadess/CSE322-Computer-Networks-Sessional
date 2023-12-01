#include <bits/stdc++.h>
using namespace std;

#define RESET "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"

double p;
int m;
string dta;
string gen;

vector<int> genBlock; // the generator.............
vector<int> checkSum; // checksum.....................

int dataBlocks[1000][1000];
vector<int> hammingBlocks[1000];  // hamming codes in sender side...........................
vector<int> serializedData;       // serialized hamming codes in sender side...................
vector<int> transmittedData;      // the data with checksum
vector<int> alteredData;          // data after transmission
vector<int> ifChanged;            // tracks the errornous bits......
vector<int> receiverBlocks[1000]; // hamming blocks in receiver side...............
vector<int> retrievedBlocks[1000];

bool randomChoose()
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    double random_value = distribution(gen);

    // Choose between 0 and 1 based on the probability
    bool result = (random_value < p) ? true : false;

    return result;
}

void takeInput()
{
    cout << "enter data string: ";
    std::getline(std::cin, dta);
    cout << endl;
    cout << "enter the number of data bytes in a row (m): ";
    cin >> m;
    cout << endl;
    cout << "enter probability (p): ";
    cin >> p;
    cout << endl;
    cout << "enter generator polynomial: ";
    cin >> gen;
    cout << endl;
    cout << endl;

    while (dta.length() % m != 0)
    {
        dta = dta + "~";
    }

    cout << "data string after padding: ";
    cout << dta;
    cout << endl;

    int i = 0;
    while (i * m < dta.length() * 8)
    {
        for (int j = 0; j < m; j++)
        {
            int intValue = static_cast<int>(dta[i * m + j]);
            int bits[8];
            for (int ii = 0; ii < 8; ++ii)
            {
                bits[ii] = (intValue >> ii) & 1;
            }
            for (int ii = 0; ii < 8; ii++)
            {
                dataBlocks[i][j * 8 + ii] = bits[7 - ii];
            }
        }
        i++;
    }

    for (int i = 0; i < gen.length(); i++)
    {
        if (gen[i] == '1')
            genBlock.push_back(1);
        else
            genBlock.push_back(0);
    }

    for (int i = 0; i < genBlock.size(); i++)
        cout << genBlock[i];
    cout << endl;

    cout << "Data blocks. m character per row" << endl;

    for (int i = 0; i < dta.length() / m; i++)
    {
        for (int j = 0; j < m * 8; j++)
        {
            cout << dataBlocks[i][j];
        }
        cout << endl;
    }
}

void makeString()
{
    for (int i = 0; i < dta.length() / m; i++)
    {
        int j = 0;
        while (j < 8 * m)
        {
            int p = 0;
            for (int k = 0; k < 8; k++)
            {
                p = p * 2;
                p += dataBlocks[i][j + k];
            }
            j += 8;
            cout << (char)p;
        }
    }
    cout << endl;
}

// in this function, we see if a number(pos) has that checkbit....
// for example, if checkbit is 8 and pos is 9, this function will return true as 9=8+1.......
bool ifPos(int checkBit, int pos)
{
    // for checkbit=8........cont=3
    int y = checkBit, cont = 0;
    while (y != 1)
    {
        cont++;
        y = y / 2;
    }

    vector<int> bitArr;
    int num = pos;
    while (num > 0)
    {
        bitArr.push_back(num & 1);
        num >>= 1;
    }
    if (bitArr[cont] == 1)
        return true;
    else
        return false;
}

void hammed()
{
    for (int i = 0; i < dta.length() / m; i++)
    {
        // build the hamming code array...................
        hammingBlocks[i].push_back(0);
        int currP = 2, currM = 0;
        while (currM < m * 8)
        {
            if (hammingBlocks[i].size() + 1 == currP)
            {
                hammingBlocks[i].push_back(0);
                currP *= 2;
            }
            else
            {
                hammingBlocks[i].push_back(dataBlocks[i][currM]);
                currM++;
            }
        }

        // replace check bits with 0 or 1 appropriately..............
        currP = 1;
        for (int j = 0; j < hammingBlocks[i].size(); j++)
        {
            if (j + 1 == currP)
            {
                // this is a check bit................
                int xorValue = 0;
                for (int k = j + 1; k < hammingBlocks[i].size(); k++)
                {
                    if (ifPos(j + 1, k + 1))
                    {
                        xorValue = xorValue ^ hammingBlocks[i][k];
                    }
                }
                hammingBlocks[i][j] = xorValue;
                currP *= 2;
            }
        }
    }

    cout << "hamming code:" << endl;

    for (int i = 0; i < dta.length() / m; i++)
    {
        int currP = 1;
        for (int j = 0; j < hammingBlocks[i].size(); j++)
        {
            if (j + 1 == currP)
            {
                cout << GREEN << hammingBlocks[i][j] << RESET;
                currP *= 2;
            }
            else
            {
                cout << hammingBlocks[i][j];
            }
        }
        cout << endl;
    }
}

void serialize()
{
    for (int i = 0; i < hammingBlocks[0].size(); i++)
    {
        for (int j = 0; j < dta.length() / m; j++)
        {
            serializedData.push_back(hammingBlocks[j][i]);
        }
    }
    for (int i = 0; i < genBlock.size() - 1; i++)
        serializedData.push_back(0);
    cout << "serialized and ready for reminder calculation" << endl;

    for (int i = 0; i < serializedData.size(); i++)
    {
        cout << serializedData[i];
    }
    cout << endl;
}

void genCheckSum()
{
    vector<int> curr;
    for (int i = 0; i < serializedData.size(); i++)
        curr.push_back(serializedData[i]);
    for (int i = 0; i < curr.size(); i++)
    {
        if (curr[i] == 0 && i < curr.size())
            ;
        else if (i + genBlock.size() - 1 >= curr.size())
        {
            for (int k = i; k < curr.size(); k++)
                checkSum.push_back(curr[k]);
            break;
        }
        else
        {
            for (int k = 0; k < genBlock.size(); k++)
            {
                curr[i + k] = genBlock[k] ^ curr[i + k];
            }
        }
    }
    cout << "reminder:" << endl;
    for (int i = 0; i < checkSum.size(); i++)
        cout << checkSum[i];
    cout << endl
         << endl;
}

void readyToTransmit()
{
    for (int i = 0; i < checkSum.size(); i++)
        serializedData.pop_back();
    for (int i = 0; i < checkSum.size(); i++)
        serializedData.push_back(checkSum[i]);

    cout << "transmitted data: " << endl;
    for (int i = 0; i < serializedData.size() - checkSum.size(); i++)
    {
        cout << serializedData[i];
        transmittedData.push_back(serializedData[i]);
    }
    for (int i = 0; i < checkSum.size(); i++)
    {
        transmittedData.push_back(checkSum[i]);
        cout << CYAN << checkSum[i] << RESET;
    }
    cout << endl
         << endl;
}

void alterData()
{
    for (int i = 0; i < transmittedData.size(); i++)
    {
        bool ans = randomChoose();
        if (ans)
        {
            alteredData.push_back(1 - transmittedData[i]);
            ifChanged.push_back(1);
        }
        else
        {
            alteredData.push_back(transmittedData[i]);
            ifChanged.push_back(0);
        }
    }
    cout << "altered data after transmission: " << endl;
    for (int i = 0; i < alteredData.size(); i++)
    {
        if (ifChanged[i])
            cout << RED << alteredData[i] << RESET;
        else
            cout << alteredData[i];
    }
    cout << endl
         << endl;
}

void errorDetection()
{
    vector<int> curr;
    for (int i = 0; i < alteredData.size(); i++)
        curr.push_back(alteredData[i]);
    vector<int> temp;
    for (int i = 0; i < curr.size(); i++)
    {
        if (curr[i] == 0 && i < curr.size())
            ;
        else if (i + genBlock.size() - 1 >= curr.size())
        {
            for (int k = i; k < curr.size(); k++)
                temp.push_back(curr[k]);
            break;
        }
        else
        {
            for (int k = 0; k < genBlock.size(); k++)
            {
                curr[i + k] = genBlock[k] ^ curr[i + k];
            }
        }
    }
    // for (int i = 0; i < temp.size(); i++)
    //     cout << temp[i];
    // cout << endl;
    if (temp.size() != 0)
        cout << "Error detected" << endl;
    else
        cout << "no error" << endl;
}

void deSerialize()
{
    for (int i = 0; i < hammingBlocks[0].size(); i++)
    {
        for (int j = 0; j < dta.length() / m; j++)
        {
            receiverBlocks[j].push_back(alteredData[i * dta.length() / m + j]);
        }
    }
    cout << endl
         << "de serialized:" << endl;

    for (int i = 0; i < dta.length() / m; i++)
    {
        for (int j = 0; j < receiverBlocks[0].size(); j++)
        {
            if (ifChanged[j * dta.length() / m + i] == 1)
                cout << RED << receiverBlocks[i][j] << RESET;
            else
                cout << receiverBlocks[i][j];
        }
        cout << endl;
    }
    cout << endl;
}

void errorCorrection()
{
    cout << endl
         << "corrected:" << endl;

    for (int d = 0; d < dta.length() / m; d++)
    {
        // at first, lets see, which check bits have error..................
        int cont = 0;
        int currP = 1;
        for (int i = 0; i < receiverBlocks[d].size(); i++)
        {
            if (i + 1 == currP)
            {
                int u = 0;
                for (int j = i + 1; j < receiverBlocks[d].size(); j++)
                {
                    if (ifPos(i + 1, j + 1))
                    {
                        u = u ^ receiverBlocks[d][j];
                    }
                }
                if (u != hammingBlocks[d][i])
                    cont += i + 1;
                currP *= 2;
            }
        }

        // now lets see, if every checkbit for a position says it is wrong, then we will correct it.

        for (int i = 0; i < receiverBlocks[d].size(); i++)
        {
            if (i + 1 == cont)
            {
                cout << YELLOW << 1 - receiverBlocks[d][i] << RESET;
                receiverBlocks[d][i] = 1 - receiverBlocks[d][i];
            }
            else if (ifChanged[i * dta.length() / m + d])
            {
                cout << RED << receiverBlocks[d][i] << RESET;
            }
            else
                cout << receiverBlocks[d][i];
        }
        cout << endl;
    }
}

void retrieve()
{

    for (int i = 0; i < dta.length() / m; i++)
    {
        int currP = 1;
        for (int j = 0; j < receiverBlocks[i].size(); j++)
        {
            if (j + 1 == currP)
                currP *= 2;
            else
            {
                retrievedBlocks[i].push_back(receiverBlocks[i][j]);
            }
        }
    }
    cout << endl
         << "check bit removed:" << endl;
    for (int i = 0; i < dta.length() / m; i++)
    {

        for (int j = 0; j < retrievedBlocks[i].size(); j++)
        {
            cout << retrievedBlocks[i][j];
        }
        cout << endl;
    }
}

void messageFound()
{
    cout << "MESSSAGE: ";
    for (int i = 0; i < dta.length() / m; i++)
    {

        for (int j = 0; j < retrievedBlocks[i].size(); j += 8)
        {
            int p = 0, l = 128;
            for (int k = j; k <= j + 7; k++)
            {

                if (retrievedBlocks[i][k] == 1)
                    p += l;
                l /= 2;
            }

            cout << (char)p;
        }
    }
    cout << endl;
}

int main()
{
    takeInput();
    makeString();
    hammed();
    serialize();
    genCheckSum();
    readyToTransmit();
    alterData();
    errorDetection();
    deSerialize();
    cout << endl;
    errorCorrection();
    cout << endl;
    retrieve();
    cout << endl;
    messageFound();
}