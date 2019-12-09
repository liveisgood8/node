const message = require("message");
const nodemailer = require("nodemailer");


async function main() { 
    // create reusable transporter object using the default SMTP transport
    let transporter = nodemailer.createTransport({
        // host: "13.1.77.8",
        // port: 25,
        // secure: false // true for 465, false for other ports
        service: "Yandex",
        auth: {
            user: "login",  // to be replaced by actual username and password
            pass: "password"
        }
    });

    // send mail with defined transport object
    let info = await transporter.sendMail({
        from: 'from@example.ru', // Адрес отправителя
        to: "to@example.com, to2@example.com", // Список адресов получаетелй
        subject: "Hello ✔", // Тема письма
        text: "Hello world?", // Текст письма
        attachments: [ // Вложения
            {
                filename: "test.xlsx",
                path: './test.xlsx'
            }
        ]
    });

    console.log("Message sent: %s", info.messageId); // Идентификатор письма
}

main().catch(console.error);