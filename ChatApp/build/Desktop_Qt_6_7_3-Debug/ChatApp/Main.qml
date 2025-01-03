import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: qsTr(username)
    property string username: "";
    // Chuyển đổi giữa các màn hình
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: loginPage
    }

    Component {
        id: loginPage
        Page {
            title: "Login/Register"

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10

                TextField {
                    id: usernameField
                    placeholderText: "Username"
                    Layout.fillWidth: true
                }

                TextField {
                    id: passwordField
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    Button {
                        text: "Login"
                        Layout.fillWidth: true
                        onClicked: {
                            chatClient.authenticate("LOGIN", usernameField.text, passwordField.text)
                            username = usernameField.text
                        }
                    }

                    Button {
                        text: "Register"
                        Layout.fillWidth: true
                        onClicked: {
                            chatClient.authenticate("REGISTER", usernameField.text, passwordField.text)
                        }
                    }
                }

                Text {
                    id: statusText
                    color: "red"
                }
            }

            Connections {
                target: chatClient
                onAuthenticationResult: {
                    if (success) {
                        stackView.push(chatPage)
                    }
                    statusText.text = message
                }
            }
        }
    }

    Component {
        id: chatPage
        Page {
            title: "Chat"

            RowLayout {
                anchors.fill: parent

                // Danh sách người dùng
                ListView {
                    id: usersListView
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: parent.height
                    model: chatClient.users
                    delegate: Rectangle {
                        width: parent.width
                        height: 40
                        border.width: 1
                        border.color: "gray"
                        property ListView __lv: ListView.view
                        color: __lv.currentIndex === index ? "lightyellow" : "white"
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                __lv.currentIndex = index;
                                chatArea.recipient = modelData.split("(")[0];
                                // console.log(chatArea.recipient);
                                chatClient.requestHistory(chatArea.recipient)
                            }
                        }
                    }
                }
                Connections {
                    target: chatClient

                    onMessagesChanged: {
                        // if(chatArea.recipient !== "" && chatClient.messages.includes(chatArea.recipient))
                        chatClient.requestHistory(chatArea.recipient)
                        chatHistory.text = chatClient.messages.join("\n")
                        // }
                    }
                }
                // Phần chat chính
                Rectangle {
                    id: chatArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#f0f0f0"

                    property string recipient: "";
                    ColumnLayout {
                        width: parent.width
                        height: parent.height
                        spacing: 10
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            TextArea {
                                id: chatHistory
                                readOnly: true
                                wrapMode: Text.Wrap
                                text: ""
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            TextField {
                                id: messageInput
                                placeholderText: "Type your message here..."
                                Layout.fillWidth: true
                            }

                            Button {
                                text: "Send"
                                onClicked: {
                                    if (messageInput.text !== "") {
                                        chatClient.sendMessage(chatArea.recipient, messageInput.text)
                                        chatHistory.text += "\nMe: " + messageInput.text
                                        messageInput.text = ""
                                    }
                                }
                            }

                        }
                    }

                }
            }
        }
    }
}
