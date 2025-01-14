#include "Server.h"

int Server::DeleteIndex;
ADDRINFO *Server::addrResult = NULL;
SOCKET Server::ClientSocket = INVALID_SOCKET;
SOCKET Server::ListenSocket = INVALID_SOCKET;
char Server::SendBuffer[2048];
char Server::recvBuffer[512];
WSADATA Server::wsaData;
ADDRINFO Server::hints;


int Server::Initialize() {

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed";
        return -1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; //семейтво протоколов //ipv4 ethernet
    hints.ai_socktype = SOCK_STREAM; //тип сокета
    hints.ai_protocol = IPPROTO_TCP; //tcp протокол
    hints.ai_flags = AI_PASSIVE;


    //инит адреса
    if (getaddrinfo(NULL, "666", &hints, &addrResult) != 0) {
        //"GetAddrInfo failed";
        freeaddrinfo(addrResult);
        WSACleanup();
        return -2;
    }

    ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        // "ListenSocket creation failed";
        freeaddrinfo(addrResult);
        WSACleanup();
        return -3;
    }


    if (bind(ListenSocket, addrResult->ai_addr, (int) addrResult->ai_addrlen) == SOCKET_ERROR) {
        //"Bind socket failed";
        closesocket(ListenSocket);
        ListenSocket = INVALID_SOCKET;
        freeaddrinfo(addrResult);
        WSACleanup();
        return -4;
    }
}

int Server::Listen() {

    int Result;

    //Start Listen
    Result = listen(ListenSocket, SOMAXCONN);
    if (Result == SOCKET_ERROR) {
        cout << "Listening socket failed" << endl;
        closesocket(ListenSocket);
        freeaddrinfo(addrResult);
        WSACleanup();
        return -5;
    }

    //Set Client connection to ClientSocket from Server side
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "Accepting socket failed" << endl;
        closesocket(ListenSocket);
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    //Close Listen Socket
    closesocket(ListenSocket);


    do {
        memset(recvBuffer, 0, 512);

        ZeroMemory(recvBuffer, 512);
        Result = recv(ClientSocket, recvBuffer, 512, 0);
        if (Result > 0) {
            cout << "Recieved: " << recvBuffer << endl;
            char Message[2048];
            memset(Message, 0, 2048);
            memset(SendBuffer, 0, 2048);


            //USER DELETE CASE----------------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'U' and recvBuffer[1] == 'D' and recvBuffer[2] == 'E' and recvBuffer[3] == 'L') {
                cout << "Initialize deleting by index\n";
                if (isdigit(recvBuffer[5]) and isdigit(recvBuffer[6])) {
                    char str[2];
                    str[0] = recvBuffer[5];
                    str[1] = recvBuffer[6];
                    DeleteIndex = atoi(str);
                } else {
                    DeleteIndex = recvBuffer[5] - 48;
                }
                Result = Database::DeleteUser(DeleteIndex);
                if (Result == 1) {
                    cout << "Successfully delete user by index = " << DeleteIndex << "\n";
                    strcpy(Message, "SUCCESSFUL_DELETE");
                } else if (Result == 0) {
                    cout << "Cant find user by this index\n";
                    strcpy(Message, "NOTFOUND");
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //USER FIND CASE------------------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'U' and recvBuffer[1] == 'F' and recvBuffer[2] == 'N' and recvBuffer[3] == 'D') {
                char sliced_buf[26];
                substr(sliced_buf, recvBuffer, 5, 26);
                string Credentials(sliced_buf);
                string Login;
                string Password;
                int i = 0;
                while (Credentials[i] != '%') {
                    Login.push_back(Credentials[i]);
                    i++;
                }
                i += 1;
                int c = i;
                i = 0;
                while (Credentials[c + i] != '%') {
                    Password.push_back(Credentials[c + i]);
                    i++;
                }
                cout << "Received login: " << Login << "  Received password: " << Password << '\n';


                string Buffer = Database::FindUser(Login, Password);

                if (Buffer == "WRONGPASS") {
                    strcpy(Message, "WRONGPASS");
                } else if (Buffer == "NOTFOUND") {
                    strcpy(Message, "NOTFOUND");
                } else {
                    strcpy(Message, Buffer.c_str());
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //USER REGISTRATION CASE-----------------------------------------------------------------------------------
            if (recvBuffer[0] == 'U' and recvBuffer[1] == 'R' and recvBuffer[2] == 'E' and recvBuffer[3] == 'G') {
                char sliced_buf[25];
                string Login;
                string Password;
                substr(sliced_buf, recvBuffer, 5, 25);
                string Credentials(sliced_buf);
                int i = 0;
                while (Credentials[i] != '%') {
                    Login.push_back(Credentials[i]);
                    i++;
                }
                i += 1;
                int c = i;
                i = 0;
                while (Credentials[c + i] != '%') {
                    Password.push_back(Credentials[c + i]);
                    i++;
                }
                cout << "Received login: " << Login << "  Received password: " << Password << '\n';

                string Buffer = Database::FindUser(Login, Password);

                if (Buffer == "NOTFOUND") {
                    Database::RegUser(Login, Password);
                    strcpy(Message, "REGISTERED                             \n");
                } else {
                    strcpy(Message, "Username is taken");
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //DATABASE ADD CASE----------------------------------------------------------------------------------------
            //DADD DCLASS%08.05.2024/18:23%12%NONE%
            if (recvBuffer[0] == 'D' and recvBuffer[1] == 'A' and recvBuffer[2] == 'D' and recvBuffer[3] == 'D') {
                char sliced_buf[40];
                substr(sliced_buf, recvBuffer, 5, 45);
                string ReservedData(sliced_buf);
                stringstream ss(ReservedData);
                string Type, Date, Place, UserLogin;
                getline(ss, Type, '%');
                getline(ss, Date, '%');
                getline(ss, Place, '%');
                getline(ss, UserLogin, '%');
                cout << Type << ' ' << Date << ' ' << Place << ' ' << UserLogin << '\n';

                string Buffer = Database::FindReservedData(Type, Date, Place);

                if (Buffer == "NOTFOUND") {
                    Database::AddNewReservation(Type, Date, Place, UserLogin);
                    strcpy(Message, "RESERVATION ADDED              \n");
                } else {
                    strcpy(Message, "This reservation is already exist");
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //DATABASE DEL CASE----------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'D' and recvBuffer[1] == 'D' and recvBuffer[2] == 'E' and recvBuffer[3] == 'L') {
                if (isdigit(recvBuffer[5]) and isdigit(recvBuffer[6])) {
                    char str[2];
                    str[0] = recvBuffer[5];
                    str[1] = recvBuffer[6];
                    DeleteIndex = atoi(str);
                } else {
                    DeleteIndex = recvBuffer[5] - 48;
                }
                cout << DeleteIndex << ' ';
                Result = Database::DeleteData(DeleteIndex);
                if (Result == 1) {
                    cout << "Successfully delete reservation\n";
                    strcpy(Message, "Successfully delete reservation");
                } else if (Result == 0) {
                    cout << "Reservation not found\n";
                    strcpy(Message, "Reservation not found");
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //DATABASE FIND CASE---------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'D' and recvBuffer[1] == 'F' and recvBuffer[2] == 'N' and recvBuffer[3] == 'D') {
                char sliced_buf[50];
                substr(sliced_buf, recvBuffer, 5, 50);
                string ReservedData(sliced_buf);
                stringstream ss(ReservedData);
                string Type, Date, Place, UserLogin;
                getline(ss, Type, '%');
                getline(ss, Date, '%');
                getline(ss, Place, '%');
                getline(ss, UserLogin, '%');
                cout << Type << ' ' << Date << ' ' << Place << ' ' << UserLogin << '\n';

                string Buffer = Database::FindReservedData(Type, Date, Place);

                if (Buffer == "NOTFOUND") {
                    strcpy(Message, "NOTFOUND");
                    cout << Message;
                } else {
                    strcpy(Message, Buffer.c_str());
                    cout << Message << '\n';
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //RESERVE CASE---------------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'R' and recvBuffer[1] == 'E' and recvBuffer[2] == 'S') {
                char sliced_buf[20];
                substr(sliced_buf, recvBuffer, 4, 20);
                string Buffer(sliced_buf);
                cout << Buffer << endl;
                string strIndex;
                string Login;

                stringstream ss(Buffer);
                ss >> strIndex;
                ss >> Login;

                int ResIndex = atoi(strIndex.c_str());
                Login.erase(std::prev(Login.end()));
                cout << atoi(strIndex.c_str()) << endl << Login;;


                Buffer = Database::Reserve(Login, ResIndex);

                if (Buffer == "RESERVED BY ANOTHER USER") {
                    cout << "RESERVED BY ANOTHER USER";
                    strcpy(Message, Buffer.c_str());
                } else if (Buffer == "ALREADY RESERVED") {
                    cout << "ALREADY RESERVED";
                    strcpy(Message, Buffer.c_str());
                } else if (Buffer == "NOTFOUND") {
                    cout << "NOTFOUND";
                    strcpy(Message, Buffer.c_str());
                } else {
                    //cout << Buffer << '\n';
                    strcpy(Message, Buffer.c_str());
                }
            }
            //---------------------------------------------------------------------------------------------------------
            //GET ALL RESERVATIONS ------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'D' and recvBuffer[1] == 'A' and recvBuffer[2] == 'L' and recvBuffer[3] == 'L') {

                string Buffer;
                for (auto &ReservedData: Database::ParsedReservedData) {
                    Buffer += (to_string(ReservedData.Index) + ReservedData.Type + ReservedData.Date +
                               ReservedData.Place + ReservedData.User + "\n");
                }
                strcpy(Message, Buffer.c_str());
                cout << Message;
            }

            //---------------------------------------------------------------------------------------------------------
            //GET ALL USERS--------------------------------------------------------------------------------------------
            if (recvBuffer[0] == 'U' and recvBuffer[1] == 'A' and recvBuffer[2] == 'L' and recvBuffer[3] == 'L') {
                string AllUserData;
                for (auto &UserData: Database::ParsedUserData) {
                    AllUserData += (to_string(UserData.Index) + UserData.Login + \
                    UserData.Password + UserData.USERID + to_string(UserData.AdminFlag) + "\n");
                }
                strcpy(Message, AllUserData.c_str());
                cout << Message;
            }

            //GET ALL AVAILABLE RESERVATIONS --------------------------------------------------------------------------
            if (recvBuffer[0] == 'A' and recvBuffer[1] == 'A' and recvBuffer[2] == 'L' and recvBuffer[3] == 'L') {
                string AllAvailableRes;
                for (auto &ResData: Database::ParsedReservedData) {
                    if (ResData.User == "NONE") {
                        AllAvailableRes += (to_string(ResData.Index) + " " + ResData.Type + " " + ResData.Date + \
                        " " + ResData.Place + " " + ResData.User + '\n');
                    }
                }
                cout << AllAvailableRes;
                strcpy(Message, AllAvailableRes.c_str());
                cout << Message;
            }
            //---------------------------------------------------------------------------------------------------------

            if (recvBuffer[0] == 'D' and recvBuffer[1] == 'F' and recvBuffer[2] == 'F' and recvBuffer[3] == 'R') {

                char sliced_buf[25];
                substr(sliced_buf, recvBuffer, 5, 12);
                string Buf(sliced_buf);
                cout << Buf;
                string Login;

                int i = 0;
                while (Buf[i] != '%') {
                    Login.push_back(Buf[i]);
                    i++;
                }

                string AllUserRes;

                for (auto &ResData: Database::ParsedReservedData) {
                    if (ResData.User == Login) {
                        AllUserRes += (to_string(ResData.Index) + ResData.Type + ResData.Date + ResData.Place +
                                       ResData.User + '\n');
                    }
                }
                if (AllUserRes.empty()) {
                    strcpy(Message, "empty");
                } else {
                    strcpy(Message, AllUserRes.c_str());
                }
            }

            cout << Message << endl;
            strcpy(SendBuffer, Message);

            Result = send(ClientSocket, SendBuffer, (int) strlen(SendBuffer), 0);
            memset(recvBuffer, 0, 512);
            memset(SendBuffer, 0, 2048);
            memset(Message, 0, 2048);


            if (Result == SOCKET_ERROR) {
                cout << "Failed to send data back" << endl;
                closesocket(ClientSocket);
                freeaddrinfo(addrResult);
                WSACleanup();
                return 1;
            }
        } else if (Result == 0) {
            cout << "Connection closing...";
        } else if (Result < 0) {
            cout << "recv failed with error";
            closesocket(ClientSocket);
            freeaddrinfo(addrResult);
            WSACleanup();
            return 1;
        }
    } while (Result > 0);

    Result = shutdown(ClientSocket, SD_SEND);
    if (Result == SOCKET_ERROR) {
        cout << "shutdown client socket failed" << endl;
        closesocket(ClientSocket);
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    closesocket(ClientSocket);
    freeaddrinfo(addrResult);
    WSACleanup();
    return 0;

}