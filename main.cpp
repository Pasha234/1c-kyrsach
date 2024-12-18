#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/datectrl.h>
#include <wx/combobox.h>
#include <wx/listctrl.h>
#include <wx/textdlg.h>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <sstream>

class ResultsFrame : public wxFrame
{
private:
    wxGrid* gridOrders;
    wxButton* btnBack;
    wxFrame* parentFrame;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;

    void OnBack(wxCommandEvent& event)
    {
        parentFrame->Show(true);
        this->Close();
    }

    void LoadOrdersData()
    {
        SQLRETURN ret;
        std::wstring query = L"SELECT * FROM Заказы";
        SQLCHAR id[10], orderNumber[50], createdAt[50];
        SQLLEN cbID, cbOrderNumber, cbCreatedAt;
        int row = 0;

        ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        if (SQL_SUCCEEDED(ret))
        {
            ret = SQLExecDirect(hStmt, (SQLWCHAR*) query.c_str(), SQL_NTS);
            if (SQL_SUCCEEDED(ret))
            {
                while (SQLFetch(hStmt) == SQL_SUCCESS)
                {
                    SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), &cbID);
                    SQLGetData(hStmt, 2, SQL_C_CHAR, orderNumber, sizeof(orderNumber), &cbOrderNumber);
                    SQLGetData(hStmt, 3, SQL_C_CHAR, createdAt, sizeof(createdAt), &cbCreatedAt);

                    gridOrders->AppendRows(1);
                    gridOrders->SetCellValue(row, 0, wxString::FromUTF8((char*)id));
                    gridOrders->SetCellValue(row, 1, wxString::FromUTF8((char*)orderNumber));
                    gridOrders->SetCellValue(row, 2, wxString::FromUTF8((char*)createdAt));
                    row++;
                }
            }
            else
            {
                wxMessageBox("Ошибка выполнения запроса к базе данных.", "Ошибка", wxICON_ERROR);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else
        {
            wxMessageBox("Ошибка выделения ресурсов для запроса.", "Ошибка", wxICON_ERROR);
        }
    }

public:
    ResultsFrame(wxFrame* parent, SQLHDBC dbc) : wxFrame(nullptr, wxID_ANY, "Результаты заказов", wxDefaultPosition, wxSize(600, 400)), parentFrame(parent), hDbc(dbc)
    {
        wxPanel* panel = new wxPanel(this);
        gridOrders = new wxGrid(panel, wxID_ANY, wxPoint(20, 20), wxSize(550, 300));
        btnBack = new wxButton(panel, wxID_ANY, "Назад", wxPoint(20, 340));

        gridOrders->CreateGrid(0, 3);
        gridOrders->SetColLabelValue(0, "ID");
        gridOrders->SetColLabelValue(1, "Order Number");
        gridOrders->SetColLabelValue(2, "Created At");

        btnBack->Bind(wxEVT_BUTTON, &ResultsFrame::OnBack, this);
        LoadOrdersData();
    }
};

class OrdersFrame : public wxFrame
{
private:
    wxButton* btnShowOrders;
    SQLHENV hEnv;
    SQLHDBC hDbc;

    void OnShowOrders(wxCommandEvent& event)
    {
        ResultsFrame* resultsFrame = new ResultsFrame(this, hDbc);
        resultsFrame->Show(true);
        this->Hide();
    }

    void InitDatabase()
    {
        SQLRETURN ret;
        SQLWCHAR connStr[] = L"DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=C:\\Users\\Павел\\Desktop\\Курсач\\DB.accdb;";
        SQLWCHAR outConnStr[1024];
        SQLSMALLINT outConnStrLen;

        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

        ret = SQLDriverConnect(hDbc, NULL, connStr, SQL_NTS, outConnStr, sizeof(outConnStr), &outConnStrLen, SQL_DRIVER_COMPLETE);
        if (!SQL_SUCCEEDED(ret))
        {
            wxMessageBox("Не удалось подключиться к базе данных. Проверьте путь к файлу и ODBC драйвер.", "Ошибка подключения", wxICON_ERROR);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
            exit(1);
        }
    }

public:
    OrdersFrame() : wxFrame(nullptr, wxID_ANY, "Список заказов", wxDefaultPosition, wxSize(400, 200))
    {
        wxPanel* panel = new wxPanel(this);
        btnShowOrders = new wxButton(panel, wxID_ANY, "Сформировать список заказов", wxPoint(100, 50));
        btnShowOrders->Bind(wxEVT_BUTTON, &OrdersFrame::OnShowOrders, this);
        InitDatabase();
    }

    ~OrdersFrame()
    {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
};

// Функция для обработки ошибок ODBC
void handleODBCError(SQLHANDLE handle, SQLSMALLINT handleType) {
    SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER nativeError;
    SQLSMALLINT i = 1, messageLength;

    while (SQLGetDiagRec(handleType, handle, i++, sqlState, &nativeError,
        message, sizeof(message), &messageLength) == SQL_SUCCESS) {
        wxLogError("ODBC Error: %s - %s", sqlState, message);
    }
}

class OrderDialog : public wxDialog {
public:
    OrderDialog(wxWindow* parent, SQLHENV env, SQLHDBC dbc)
        : wxDialog(parent, wxID_ANY, "New Order", wxDefaultPosition, wxDefaultSize), env(env), dbc(dbc) {

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        wxGridSizer* gridSizer = new wxGridSizer(2, wxSize(5, 5));

        gridSizer->Add(new wxStaticText(this, wxID_ANY, "Order Number:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        orderNumberCtrl = new wxTextCtrl(this, wxID_ANY);
        gridSizer->Add(orderNumberCtrl, 0, wxEXPAND);

        gridSizer->Add(new wxStaticText(this, wxID_ANY, "Creation Date:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        creationDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DEFAULT | wxDP_SHOWCENTURY);
        gridSizer->Add(creationDateCtrl, 0, wxEXPAND);

        gridSizer->Add(new wxStaticText(this, wxID_ANY, "Update Date:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        updateDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DEFAULT | wxDP_SHOWCENTURY);
        gridSizer->Add(updateDateCtrl, 0, wxEXPAND);

        mainSizer->Add(gridSizer, 0, wxALL | wxEXPAND, 10);

        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        wxButton* saveButton = new wxButton(this, wxID_OK, "Save");
        wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
        buttonSizer->Add(saveButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);
        mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

        SetSizerAndFit(mainSizer);

        Bind(wxEVT_BUTTON, &OrderDialog::OnSave, this, wxID_OK);
    }

private:
    void OnSave(wxCommandEvent& event) {
        wxString orderNumber = orderNumberCtrl->GetValue();
        wxDateTime creationDate = creationDateCtrl->GetValue();
        wxDateTime updateDate = updateDateCtrl->GetValue();

        wxString creationDateStr = wxString::Format("%02d.%02d.%04d", creationDate.GetDay(), creationDate.GetMonth() + 1, creationDate.GetYear());
        wxString updateDateStr = wxString::Format("%02d.%02d.%04d", updateDate.GetDay(), updateDate.GetMonth() + 1, updateDate.GetYear());

        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

        wxString query = "INSERT INTO Заказы (OrderNumber, CreatedAt, UpdatedAt) VALUES (?, ?, ?)";

        // Конвертация wxString в SQLWCHAR* с использованием std::vector (Безопасный способ)
        std::wstring stdQuery = query.ToStdWstring();
        std::vector<SQLWCHAR> sqlQueryVec(stdQuery.begin(), stdQuery.end());
        sqlQueryVec.push_back(L'\0');
        SQLWCHAR* sqlQuery = sqlQueryVec.data();

        SQLPrepare(stmt, sqlQuery, SQL_NTS);


        // Конвертация номера заказа
        std::wstring stdOrderNumber = orderNumber.ToStdWstring();
        std::vector<SQLWCHAR> sqlOrderNumberVec(stdOrderNumber.begin(), stdOrderNumber.end());
        sqlOrderNumberVec.push_back(L'\0');
        SQLWCHAR* sqlOrderNumber = sqlOrderNumberVec.data();

        //Конвертация дат
        std::wstring stdCreationDate = creationDateStr.ToStdWstring();
        std::vector<SQLWCHAR> sqlCreationDateVec(stdCreationDate.begin(), stdCreationDate.end());
        sqlCreationDateVec.push_back(L'\0');
        SQLWCHAR* sqlCreationDate = sqlCreationDateVec.data();

        std::wstring stdUpdateDate = updateDateStr.ToStdWstring();
        std::vector<SQLWCHAR> sqlUpdateDateVec(stdUpdateDate.begin(), stdUpdateDate.end());
        sqlUpdateDateVec.push_back(L'\0');
        SQLWCHAR* sqlUpdateDate = sqlUpdateDateVec.data();

        // Привязка параметров
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, stdOrderNumber.length(), 0, sqlOrderNumber, 0, nullptr);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, stdCreationDate.length(), 0, sqlCreationDate, 0, nullptr);
        SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, stdUpdateDate.length(), 0, sqlUpdateDate, 0, nullptr);

        SQLRETURN ret = SQLExecute(stmt);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            handleODBCError(stmt, SQL_HANDLE_STMT);
            wxMessageBox("Error saving order.", "Error", wxOK | wxICON_ERROR);
        }
        else {
            wxMessageBox("Order saved successfully.", "Success", wxOK | wxICON_INFORMATION);
        }

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        EndModal(wxID_OK);
    }

    wxTextCtrl* orderNumberCtrl;
    wxDatePickerCtrl* creationDateCtrl;
    wxDatePickerCtrl* updateDateCtrl;
    SQLHENV env;
    SQLHDBC dbc;
};

class CargoByVehicleDialog : public wxDialog {
public:
    CargoByVehicleDialog(wxWindow* parent, SQLHENV env, SQLHDBC dbc)
        : wxDialog(parent, wxID_ANY, "Грузы по ТС", wxDefaultPosition, wxDefaultSize), env(env), dbc(dbc) {

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* vehicleTypeLabel = new wxStaticText(this, wxID_ANY, "Тип транспорта:");
        vehicleTypeComboBox = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
        mainSizer->Add(vehicleTypeLabel, 0, wxALL | wxALIGN_LEFT, 5);
        mainSizer->Add(vehicleTypeComboBox, 0, wxALL | wxEXPAND, 5);

        cargoListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
        cargoListCtrl->InsertColumn(0, "ID Груза");
        cargoListCtrl->InsertColumn(1, "Название груза");
        mainSizer->Add(cargoListCtrl, 1, wxALL | wxEXPAND, 5);

        wxButton* okButton = new wxButton(this, wxID_OK, "OK");
        wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Отмена");
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(okButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);
        mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

        SetSizerAndFit(mainSizer);

        LoadVehicleTypes();

        Bind(wxEVT_COMBOBOX, &CargoByVehicleDialog::OnVehicleTypeSelected, this, vehicleTypeComboBox->GetId());
    }

private:
    void LoadVehicleTypes() {
        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

        SQLWCHAR query[] = L"SELECT ID, Name FROM `Транспортные средства`";
        SQLExecDirect(stmt, query, SQL_NTS);

        SQLWCHAR vehicleTypeName[256];
        SQLINTEGER vehicleTypeID;
        SQLLEN ind;

        while (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLGetData(stmt, 1, SQL_C_LONG, &vehicleTypeID, 0, &ind);
            SQLGetData(stmt, 2, SQL_C_WCHAR, vehicleTypeName, sizeof(vehicleTypeName), &ind);

            if (ind != SQL_NULL_DATA) {
                wxString wxTypeName(vehicleTypeName);
                vehicleTypeComboBox->Append(wxTypeName, (void*)vehicleTypeID);
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    void OnVehicleTypeSelected(wxCommandEvent& event) {
        cargoListCtrl->DeleteAllItems();

        int selection = vehicleTypeComboBox->GetSelection();
        if (selection != wxNOT_FOUND) {
            int vehicleID = (int)(intptr_t)vehicleTypeComboBox->GetClientData(selection);

            SQLHSTMT stmt;
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

            wxString query = wxString::Format("SELECT ID, ItemDescription FROM Доставки WHERE TransportID = %d", vehicleID);
            std::wstring stdQuery = query.ToStdWstring();
            std::vector<SQLWCHAR> sqlQueryVec(stdQuery.begin(), stdQuery.end());
            sqlQueryVec.push_back(L'\0');
            SQLWCHAR* sqlQuery = sqlQueryVec.data();
            SQLExecDirect(stmt, sqlQuery, SQL_NTS);

            SQLWCHAR cargoName[256];
            SQLINTEGER cargoID;
            SQLLEN ind;

            while (SQLFetch(stmt) == SQL_SUCCESS) {
                SQLGetData(stmt, 1, SQL_C_LONG, &cargoID, 0, &ind);
                SQLGetData(stmt, 2, SQL_C_WCHAR, cargoName, sizeof(cargoName), &ind);

                if (ind != SQL_NULL_DATA) {
                    wxString wxCargoName(cargoName);
                    long itemIndex = cargoListCtrl->InsertItem(cargoListCtrl->GetItemCount(), wxString::Format("%d", cargoID));
                    cargoListCtrl->SetItem(itemIndex, 1, wxCargoName);
                }
            }

            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }
    }

    wxComboBox* vehicleTypeComboBox;
    wxListCtrl* cargoListCtrl;
    SQLHENV env;
    SQLHDBC dbc;
};

class CargoByCategoryDialog : public wxDialog {
public:
    CargoByCategoryDialog(wxWindow* parent, SQLHENV env, SQLHDBC dbc)
        : wxDialog(parent, wxID_ANY, "Грузы по категории", wxDefaultPosition, wxDefaultSize), env(env), dbc(dbc) {

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Создаем выпадающий список для выбора категории
        wxStaticText* categoryLabel = new wxStaticText(this, wxID_ANY, "Категория:");
        categoryComboBox = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
        mainSizer->Add(categoryLabel, 0, wxALL | wxALIGN_LEFT, 5);
        mainSizer->Add(categoryComboBox, 0, wxALL | wxEXPAND, 5);

        // Создаем список для отображения грузов
        cargoListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
        cargoListCtrl->InsertColumn(0, "ID Груза");
        cargoListCtrl->InsertColumn(1, "Название груза");
        mainSizer->Add(cargoListCtrl, 1, wxALL | wxEXPAND, 5);

        wxButton* okButton = new wxButton(this, wxID_OK, "OK");
        wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Отмена");
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(okButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);
        mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

        SetSizerAndFit(mainSizer);

        // Загружаем категории из базы данных
        LoadCategories();

        Bind(wxEVT_COMBOBOX, &CargoByCategoryDialog::OnCategorySelected, this, categoryComboBox->GetId());
    }

private:
    void LoadCategories() {
        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

        // Запрос для получения категорий
        SQLWCHAR query[] = L"SELECT ID, Name FROM Категории";
        SQLExecDirect(stmt, query, SQL_NTS);

        SQLWCHAR categoryName[256];
        SQLINTEGER categoryID;
        SQLLEN ind;

        while (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLGetData(stmt, 1, SQL_C_LONG, &categoryID, 0, &ind);
            SQLGetData(stmt, 2, SQL_C_WCHAR, categoryName, sizeof(categoryName), &ind);

            if (ind != SQL_NULL_DATA) {
                wxString wxCategoryName(categoryName);
                categoryComboBox->Append(wxCategoryName, (void*)categoryID);
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    void OnCategorySelected(wxCommandEvent& event) {
        cargoListCtrl->DeleteAllItems();

        int selection = categoryComboBox->GetSelection();
        if (selection != wxNOT_FOUND) {
            int categoryID = (int)(intptr_t)categoryComboBox->GetClientData(selection);

            SQLHSTMT stmt;
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

            // Запрос для получения грузов по ID категории
            wxString query = wxString::Format("SELECT ID, ItemDescription FROM Доставки WHERE CategoryID = %d", categoryID); 
            std::wstring stdQuery = query.ToStdWstring();
            std::vector<SQLWCHAR> sqlQueryVec(stdQuery.begin(), stdQuery.end());
            sqlQueryVec.push_back(L'\0');
            SQLWCHAR* sqlQuery = sqlQueryVec.data();
            SQLExecDirect(stmt, sqlQuery, SQL_NTS);

            SQLWCHAR cargoName[256];
            SQLINTEGER cargoID;
            SQLLEN ind;

            while (SQLFetch(stmt) == SQL_SUCCESS) {
                SQLGetData(stmt, 1, SQL_C_LONG, &cargoID, 0, &ind);
                SQLGetData(stmt, 2, SQL_C_WCHAR, cargoName, sizeof(cargoName), &ind);

                if (ind != SQL_NULL_DATA) {
                    wxString wxCargoName(cargoName);
                    long itemIndex = cargoListCtrl->InsertItem(cargoListCtrl->GetItemCount(), wxString::Format("%d", cargoID));
                    cargoListCtrl->SetItem(itemIndex, 1, wxCargoName);
                }
            }

            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }
    }

    wxComboBox* categoryComboBox;
    wxListCtrl* cargoListCtrl;
    SQLHENV env;
    SQLHDBC dbc;
};

class DeliveredCargoGridFrame : public wxFrame {
public:
    DeliveredCargoGridFrame(wxFrame* parent, SQLHDBC dbc)
        : wxFrame(nullptr, wxID_ANY, "Доставленные заказы", wxDefaultPosition, wxSize(600, 400)), parentFrame(parent), dbc(dbc) {

        wxPanel* panel = new wxPanel(this); // Создаем панель

        gridCargo = new wxGrid(panel, wxID_ANY, wxPoint(20, 20), wxSize(550, 300));
        okButton = new wxButton(panel, wxID_ANY, "OK", wxPoint(20, 340));

        gridCargo->CreateGrid(0, 3);
        gridCargo->SetColLabelValue(0, "ID Груза");
        gridCargo->SetColLabelValue(1, "Название груза");
        gridCargo->SetColLabelValue(2, "Дата доставки");

        okButton->Bind(wxEVT_BUTTON, &DeliveredCargoGridFrame::OnOK, this);

        LoadDeliveredCargoData();
    }

private:
    void OnOK(wxCommandEvent& event) {
        parentFrame->Show(true);
        this->Close();
    }

    void LoadDeliveredCargoData() {
        SQLHSTMT hStmt;
        SQLRETURN ret;
        std::wstring query = L"SELECT ID, ItemDescription, DeliveredAt FROM Доставки WHERE DeliveredAt IS NOT NULL";

        ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hStmt);
        if (SQL_SUCCEEDED(ret)) {
            ret = SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
            if (SQL_SUCCEEDED(ret)) {
                int row = 0;
                SQLINTEGER cargoID;
                SQLWCHAR cargoName[256];
                SQLCHAR deliveredAt[50];
                SQLLEN ind;

                while (SQLFetch(hStmt) == SQL_SUCCESS) {
                    SQLGetData(hStmt, 1, SQL_C_LONG, &cargoID, 0, &ind);
                    SQLGetData(hStmt, 2, SQL_C_WCHAR, cargoName, sizeof(cargoName), &ind);
                    SQLGetData(hStmt, 3, SQL_C_CHAR, &deliveredAt, sizeof(deliveredAt), &ind);

                    gridCargo->AppendRows(1);

                    if (ind != SQL_NULL_DATA) {
                        gridCargo->SetCellValue(row, 0, wxString::Format("%d", cargoID));
                        gridCargo->SetCellValue(row, 1, wxString(cargoName));
                        gridCargo->SetCellValue(row, 2, wxString::FromUTF8((char*)deliveredAt));
                    }
                    else {
                        gridCargo->SetCellValue(row, 2, wxString(""));
                    }
                    row++;
                }
            }
            else {
                wxMessageBox("Ошибка выполнения запроса к базе данных.", "Ошибка", wxICON_ERROR);
                handleODBCError(hStmt, SQL_HANDLE_STMT);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else {
            wxMessageBox("Ошибка выделения ресурсов для запроса.", "Ошибка", wxICON_ERROR);
            handleODBCError(dbc, SQL_HANDLE_DBC);
        }
    }

    wxGrid* gridCargo;
    wxButton* okButton;
    wxFrame* parentFrame;
    SQLHENV env;
    SQLHDBC dbc;
};

class EditOrderDialog : public wxDialog {
public:
    EditOrderDialog(wxWindow* parent, SQLHENV env, SQLHDBC dbc, int orderID)
        : wxDialog(parent, wxID_ANY, "Редактирование заказа", wxDefaultPosition, wxSize(400, 300)), env(env), dbc(dbc), orderID(orderID) {

        wxPanel* panel = new wxPanel(this);

        wxStaticText* orderNumberLabel = new wxStaticText(panel, wxID_ANY, "Номер заказа:", wxPoint(20, 20));
        orderNumberText = new wxTextCtrl(panel, wxID_ANY, "", wxPoint(150, 20), wxSize(200, -1));

        wxStaticText* createdAtLabel = new wxStaticText(panel, wxID_ANY, "Дата создания:", wxPoint(20, 60));
        createdAtPicker = new wxDatePickerCtrl(panel, wxID_ANY, wxDefaultDateTime, wxPoint(150, 60), wxSize(200, -1));

        wxStaticText* updatedAtLabel = new wxStaticText(panel, wxID_ANY, "Дата обновления:", wxPoint(20, 100));
        updatedAtPicker = new wxDatePickerCtrl(panel, wxID_ANY, wxDefaultDateTime, wxPoint(150, 100), wxSize(200, -1));


        wxButton* saveButton = new wxButton(panel, wxID_ANY, "Сохранить", wxPoint(20, 200));
        wxButton* cancelButton = new wxButton(panel, wxID_ANY, "Отмена", wxPoint(150, 200));

        saveButton->Bind(wxEVT_BUTTON, &EditOrderDialog::OnSave, this);
        cancelButton->Bind(wxEVT_BUTTON, &EditOrderDialog::OnCancel, this);

        LoadOrderData();
    }

private:
    void LoadOrderData() {
        SQLHSTMT hStmt;
        SQLRETURN ret;
        std::wstringstream queryStream;
        queryStream << L"SELECT OrderNumber, CreatedAt, UpdatedAt FROM Заказы WHERE ID = " << orderID;
        std::wstring query = queryStream.str();

        ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hStmt);
        if (SQL_SUCCEEDED(ret)) {
            ret = SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
            if (SQL_SUCCEEDED(ret)) {
                SQLWCHAR orderNumber[50];
                SQL_TIMESTAMP_STRUCT createdAt, updatedAt;
                SQLLEN ind;

                if (SQLFetch(hStmt) == SQL_SUCCESS) {
                    SQLGetData(hStmt, 1, SQL_C_WCHAR, orderNumber, sizeof(orderNumber), &ind);
                    SQLGetData(hStmt, 2, SQL_C_TYPE_TIMESTAMP, &createdAt, sizeof(createdAt), &ind);
                    SQLGetData(hStmt, 3, SQL_C_TYPE_TIMESTAMP, &updatedAt, sizeof(updatedAt), &ind);

                    orderNumberText->SetValue(wxString(orderNumber));

                    if (ind != SQL_NULL_DATA) {
                        createdAtPicker->SetValue(wxDateTime(createdAt.day, (wxDateTime::Month)(createdAt.month - 1), createdAt.year, createdAt.hour, createdAt.minute, createdAt.second));
                        updatedAtPicker->SetValue(wxDateTime(updatedAt.day, (wxDateTime::Month)(updatedAt.month - 1), updatedAt.year, updatedAt.hour, updatedAt.minute, updatedAt.second));
                    }
                }
                else {
                    wxMessageBox("Заказ не найден в базе данных.", "Ошибка", wxICON_ERROR);
                    EndModal(wxID_CANCEL);
                }
            }
            else {
                handleODBCError(hStmt, SQL_HANDLE_STMT);
                wxMessageBox("Ошибка выполнения запроса к базе данных.", "Ошибка", wxICON_ERROR);
                EndModal(wxID_CANCEL);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else {
            handleODBCError(dbc, SQL_HANDLE_DBC);
            wxMessageBox("Ошибка выделения ресурсов для запроса.", "Ошибка", wxICON_ERROR);
            EndModal(wxID_CANCEL);
        }
    }

    void OnSave(wxCommandEvent& event) {
        SQLHSTMT hStmt;
        SQLRETURN ret;
        std::wstringstream queryStream;

        wxDateTime creationDate = createdAtPicker->GetValue();
        wxDateTime updateDate = updatedAtPicker->GetValue();

        wxString creationDateStr = wxString::Format("%02d.%02d.%04d", creationDate.GetDay(), creationDate.GetMonth() + 1, creationDate.GetYear());
        wxString updateDateStr = wxString::Format("%02d.%02d.%04d", updateDate.GetDay(), updateDate.GetMonth() + 1, updateDate.GetYear());

        std::wstring stdCreationDate = creationDateStr.ToStdWstring();
        std::vector<SQLWCHAR> sqlCreationDateVec(stdCreationDate.begin(), stdCreationDate.end());
        sqlCreationDateVec.push_back(L'\0');
        SQLWCHAR* sqlCreationDate = sqlCreationDateVec.data();

        std::wstring stdUpdateDate = updateDateStr.ToStdWstring();
        std::vector<SQLWCHAR> sqlUpdateDateVec(stdUpdateDate.begin(), stdUpdateDate.end());
        sqlUpdateDateVec.push_back(L'\0');
        SQLWCHAR* sqlUpdateDate = sqlUpdateDateVec.data();

        wxString orderNumber = orderNumberText->GetValue();

        std::wstring stdOrderNumber = orderNumber.ToStdWstring();
        std::vector<SQLWCHAR> sqlOrderNumberVec(stdOrderNumber.begin(), stdOrderNumber.end());
        sqlOrderNumberVec.push_back(L'\0');
        SQLWCHAR* sqlOrderNumber = sqlOrderNumberVec.data();

        queryStream << L"UPDATE Заказы SET OrderNumber = ?, CreatedAt = ?, UpdatedAt = ? WHERE ID = " << orderID;
        std::wstring query = queryStream.str();

        ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hStmt);
        if (SQL_SUCCEEDED(ret)) {
            ret = SQLPrepare(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
            if (SQL_SUCCEEDED(ret)) {
                SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, stdOrderNumber.length(), 0, sqlOrderNumber, 0, nullptr);
                SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, stdCreationDate.length(), 0, sqlCreationDate, 0, nullptr);
                SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, stdUpdateDate.length(), 0, sqlUpdateDate, 0, nullptr);

                if (SQL_SUCCEEDED(ret)) {
                    ret = SQLExecute(hStmt);
                    if (SQL_SUCCEEDED(ret)) {
                        wxMessageBox("Заказ успешно обновлен.", "Успех", wxICON_INFORMATION);
                        EndModal(wxID_OK);
                    }
                    else {
                        handleODBCError(hStmt, SQL_HANDLE_STMT);
                        wxMessageBox("Ошибка обновления заказа.", "Ошибка", wxICON_ERROR);
                    }

                }
                else {
                    handleODBCError(hStmt, SQL_HANDLE_STMT);
                    wxMessageBox("Ошибка привязки параметров.", "Ошибка", wxICON_ERROR);
                }

            }
            else {
                handleODBCError(hStmt, SQL_HANDLE_STMT);
                wxMessageBox("Ошибка подготовки запроса.", "Ошибка", wxICON_ERROR);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else {
            handleODBCError(dbc, SQL_HANDLE_DBC);
            wxMessageBox("Ошибка выделения ресурсов для запроса.", "Ошибка", wxICON_ERROR);
        }
    }

    void OnCancel(wxCommandEvent& event) {
        EndModal(wxID_CANCEL);
    }

    wxTextCtrl* orderNumberText;
    wxDatePickerCtrl* createdAtPicker;
    wxDatePickerCtrl* updatedAtPicker;
    SQLHENV env;
    SQLHDBC dbc;
    int orderID;
};

class DeliveryInfoDialog : public wxDialog {
public:
    DeliveryInfoDialog(wxWindow* parent, SQLHENV env, SQLHDBC dbc, int orderID)
        : wxDialog(parent, wxID_ANY, "Информация о сроках доставки", wxDefaultPosition, wxSize(600, 400)), env(env), dbc(dbc), orderID(orderID) {

        wxPanel* panel = new wxPanel(this);
        gridDeliveryInfo = new wxGrid(panel, wxID_ANY, wxPoint(20, 20), wxSize(550, 300));

        gridDeliveryInfo->CreateGrid(0, 5);
        gridDeliveryInfo->SetColLabelValue(0, "Описание груза");
        gridDeliveryInfo->SetColLabelValue(1, "Срок доставки");
        gridDeliveryInfo->SetColLabelValue(2, "Доставлено");
        gridDeliveryInfo->SetColLabelValue(3, "Создано");
        gridDeliveryInfo->SetColLabelValue(4, "ID груза");

        LoadDeliveryInfo();
    }

private:
    void LoadDeliveryInfo() {
        SQLHSTMT hStmt;
        SQLRETURN ret;
        std::wstringstream queryStream;

        queryStream << L"SELECT Доставки.ItemDescription, Доставки.DeliveryDeadline, Доставки.DeliveredAt, Доставки.CreatedAt, Доставки.ID "
            L"FROM Доставки "
            L"INNER JOIN Заказы ON Доставки.OrderID = Заказы.ID "
            L"WHERE Заказы.ID = " << orderID;

        std::wstring query = queryStream.str();

        ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hStmt);
        if (SQL_SUCCEEDED(ret)) {
            ret = SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
            if (SQL_SUCCEEDED(ret)) {
                SQLWCHAR itemDescription[256];
                SQL_TIMESTAMP_STRUCT deliveryDeadline, deliveredAt, createdAt;
                SQLLEN ind;
                SQLINTEGER cargoID;

                int row = 0;
                while (SQLFetch(hStmt) == SQL_SUCCESS) {
                    SQLGetData(hStmt, 1, SQL_C_WCHAR, itemDescription, sizeof(itemDescription), &ind);
                    SQLGetData(hStmt, 2, SQL_C_TYPE_TIMESTAMP, &deliveryDeadline, sizeof(deliveryDeadline), &ind);
                    SQLGetData(hStmt, 3, SQL_C_TYPE_TIMESTAMP, &deliveredAt, sizeof(deliveredAt), &ind);
                    SQLGetData(hStmt, 4, SQL_C_TYPE_TIMESTAMP, &createdAt, sizeof(createdAt), &ind);
                    SQLGetData(hStmt, 5, SQL_C_LONG, &cargoID, 0, &ind);

                    gridDeliveryInfo->AppendRows(1);

                    gridDeliveryInfo->SetCellValue(row, 0, wxString(itemDescription));

                    if (ind != SQL_NULL_DATA) {
                        gridDeliveryInfo->SetCellValue(row, 1, FormatTimestamp(deliveryDeadline));
                        gridDeliveryInfo->SetCellValue(row, 2, FormatTimestamp(deliveredAt));
                        gridDeliveryInfo->SetCellValue(row, 3, FormatTimestamp(createdAt));
                        gridDeliveryInfo->SetCellValue(row, 4, wxString::Format("%d", cargoID));
                    }
                    row++;
                }
            }
            else {
                handleODBCError(hStmt, SQL_HANDLE_STMT);
                wxMessageBox("Ошибка выполнения запроса.", "Ошибка", wxICON_ERROR);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else {
            handleODBCError(dbc, SQL_HANDLE_DBC);
            wxMessageBox("Ошибка выделения ресурсов для запроса.", "Ошибка", wxICON_ERROR);
        }
    }

    wxString FormatTimestamp(const SQL_TIMESTAMP_STRUCT& ts) {
        if (ts.year == 0) return wxString("");
        return wxString::Format("%02d.%02d.%d %02d:%02d:%02d", ts.day, ts.month, ts.year, ts.hour, ts.minute, ts.second);
    }

    wxGrid* gridDeliveryInfo;
    SQLHENV env;
    SQLHDBC dbc;
    int orderID;
};

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title, SQLHENV env, SQLHDBC dbc) : wxFrame(nullptr, wxID_ANY, title), env(env), dbc(dbc) {
        wxPanel* panel = new wxPanel(this, wxID_ANY);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        wxButton* deliveredCargoInfoButton = new wxButton(panel, wxID_ANY, "Получить информацию о доставленных грузах");
        wxButton* generateOrdersListButton = new wxButton(panel, wxID_ANY, "Сформировать список заказов");
        wxButton* createOrderButton = new wxButton(panel, wxID_ANY, "Создать заказ");
        wxButton* editOrderButton = new wxButton(panel, wxID_ANY, "Редактировать заказ");
        wxButton* cargoByVehicleButton = new wxButton(panel, wxID_ANY, "Грузы, доставляемые определенным ТС");
        wxButton* cargoByCategoryButton = new wxButton(panel, wxID_ANY, "Грузы конкретной категории");
        wxButton* deliveryTimeInfoButton = new wxButton(panel, wxID_ANY, "Информация о сроках доставки");

        mainSizer->Add(deliveredCargoInfoButton, 0, wxALL | wxEXPAND, 5);
        mainSizer->Add(generateOrdersListButton, 0, wxALL | wxEXPAND, 5);
        mainSizer->Add(createOrderButton, 0, wxALL | wxEXPAND, 5);
        mainSizer->Add(editOrderButton, 0, wxALL | wxEXPAND, 5);
        mainSizer->Add(cargoByVehicleButton, 0, wxALL | wxEXPAND, 5);
        mainSizer->Add(cargoByCategoryButton, 0, wxALL | wxEXPAND, 5);
        mainSizer->Add(deliveryTimeInfoButton, 0, wxALL | wxEXPAND, 5);

        panel->SetSizerAndFit(mainSizer);
        Centre();

        deliveredCargoInfoButton->Bind(wxEVT_BUTTON, &MainFrame::OnDeliveredCargoInfo, this);
        generateOrdersListButton->Bind(wxEVT_BUTTON, &MainFrame::OnShowOrders, this);
        createOrderButton->Bind(wxEVT_BUTTON, &MainFrame::OnCreateOrder, this);
        editOrderButton->Bind(wxEVT_BUTTON, &MainFrame::OnEditOrder, this);
        cargoByVehicleButton->Bind(wxEVT_BUTTON, &MainFrame::OnCargoByVehicle, this);
        cargoByCategoryButton->Bind(wxEVT_BUTTON, &MainFrame::OnCargoByCategory, this);
        deliveryTimeInfoButton->Bind(wxEVT_BUTTON, &MainFrame::OnShowDeliveryInfo, this);
    }

private:
    void OnDeliveredCargoInfo(wxCommandEvent& event) {
        DeliveredCargoGridFrame* resultsFrame = new DeliveredCargoGridFrame(this, dbc);
        resultsFrame->Show(true);
        this->Hide();
    }

    void OnGenerateOrdersList(wxCommandEvent& event) {
        wxLogMessage("Сформировать список заказов");
    }

    void OnCreateOrder(wxCommandEvent& event) {
        OrderDialog dlg(this, env, dbc);
        dlg.ShowModal();
    }
    void OnEditOrder(wxCommandEvent& event) {
        wxTextEntryDialog dialog(this, "Введите номер заказа:", "Редактирование заказа");
        if (dialog.ShowModal() == wxID_OK) {
            long orderID;
            if (dialog.GetValue().ToLong(&orderID)) {
                // Проверяем, существует ли заказ с таким ID в базе данных
                if (CheckOrderExists(orderID)) {
                    EditOrderDialog editDialog(this, env, dbc, orderID);
                    editDialog.ShowModal();
                }
                else {
                    wxMessageBox("Заказ с таким номером не найден.", "Ошибка", wxICON_ERROR);
                }
            }
            else {
                wxMessageBox("Некорректный номер заказа.", "Ошибка", wxICON_ERROR);
            }
        }
    }

    bool CheckOrderExists(long orderID) {
        SQLHSTMT hStmt;
        SQLRETURN ret;
        std::wstringstream queryStream;
        queryStream << L"SELECT 1 FROM Заказы WHERE ID = " << orderID;
        std::wstring query = queryStream.str();

        ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hStmt);
        if (SQL_SUCCEEDED(ret)) {
            ret = SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
            if (SQL_SUCCEEDED(ret)) {
                if (SQLFetch(hStmt) == SQL_SUCCESS) {
                    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                    return true;
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        return false;
    }
    void OnCargoByVehicle(wxCommandEvent& event) {
        CargoByVehicleDialog dlg(this, env, dbc);
        dlg.ShowModal();
    }
    void OnCargoByCategory(wxCommandEvent& event) {
        CargoByCategoryDialog dlg(this, env, dbc);
        dlg.ShowModal();
    }
    void OnShowDeliveryInfo(wxCommandEvent& event) {
        wxTextEntryDialog dialog(this, "Введите номер заказа:", "Информация о доставке");
        if (dialog.ShowModal() == wxID_OK) {
            long orderID;
            if (dialog.GetValue().ToLong(&orderID)) {
                if (CheckOrderExists(orderID)) {
                    DeliveryInfoDialog deliveryInfoDialog(this, env, dbc, orderID);
                    deliveryInfoDialog.ShowModal();
                }
                else {
                    wxMessageBox("Заказ с таким номером не найден.", "Ошибка", wxICON_ERROR);
                }
            }
            else {
                wxMessageBox("Некорректный номер заказа.", "Ошибка", wxICON_ERROR);
            }
        }
    }
    void OnShowOrders(wxCommandEvent& event)
    {
        ResultsFrame* resultsFrame = new ResultsFrame(this, dbc);
        resultsFrame->Show(true);
        this->Hide();
    }
    SQLHENV env;
    SQLHDBC dbc;
};

class MyApp : public wxApp {
public:
    bool OnInit() override {
        SQLHENV env;
        SQLHDBC dbc;

        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        wxString connectionString = L"DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=C:\\Users\\Павел\\Desktop\\Курсач\\DB.accdb;";
        SQLRETURN ret = SQLDriverConnect(dbc, nullptr, (SQLWCHAR*)connectionString.wc_str(), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);

        if (ret != SQL_SUCCESS) {
            handleODBCError(dbc, SQL_HANDLE_DBC);
            wxLogError("Failed to connect to database.");
            return false;
        }

        MainFrame* frame = new MainFrame("Учет доставки грузов", env, dbc);
        frame->Show(true);
        return true;
    }

    int OnExit() override {
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return wxApp::OnExit();
    }

private:
    SQLHENV env;
    SQLHDBC dbc;
};

wxIMPLEMENT_APP(MyApp);
