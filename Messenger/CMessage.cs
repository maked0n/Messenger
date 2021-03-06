﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Threading;

namespace Messenger {
    enum EStatus {
        Sending,
        Sent,
        FailedToSend,
        Delivered,
        Seen,
        Incoming
    }
    class CMessage {
        private string m_user;
        private string m_content;
        private DateTime m_time;
        public string m_sender {get; set; }
        public TextRange m_message_range { get; set; }
        public TextRange m_nick_range { get; set; }
        public TextRange m_date_range { get; set; }
        public EStatus status { get; set; }
        public CMessage(string user, string sender, byte[] msg_content, CModel.EMessageType msg_type, EStatus msg_status,
            TextPointer doc_end, DateTime time, SynchronizationContext context) {
            TextRange content_range = new TextRange(doc_end, doc_end);
            m_sender = sender;
            m_user = user;
            string msg_str;
            string date_str = "<" + time.ToString() + ">";
            if (msg_type == CModel.EMessageType.Text) {
                msg_str = System.Text.Encoding.UTF8.GetString(msg_content);;
                m_content = date_str + m_sender + ": " + msg_str + "\r" ;
                
            }
            else if (msg_type == CModel.EMessageType.Image) {
                msg_str = " sent an image\r";
                m_content = date_str + m_sender + msg_str;
            }
            else {
                msg_str = " sent a video\r";
                m_content = date_str + m_sender + msg_str;
            }
            context.Send(o => { content_range.Text = m_content; 
                m_date_range = new TextRange(content_range.Start, content_range.Start.GetPositionAtOffset(date_str.Length));
                m_nick_range = new TextRange(m_date_range.End, m_date_range.End.GetPositionAtOffset(m_sender.Length + 1));
                m_message_range = new TextRange(m_nick_range.End, m_nick_range.End.GetPositionAtOffset(msg_str.Length + 1));
            }, null);
            status = msg_status;
            UpdateRepresentation(EStatus.Sending, context);
            m_time = time;
        }
        public void UpdateRepresentation(EStatus status, SynchronizationContext context) {
            BrushConverter bc = new BrushConverter();
            if (m_sender == m_user) {
                context.Send(o => { m_nick_range.ApplyPropertyValue(TextElement.ForegroundProperty, bc.ConvertFromString("Red")); }, null);
            }
            else context.Send(o => { m_nick_range.ApplyPropertyValue(TextElement.ForegroundProperty, bc.ConvertFromString("Blue")); }, null);
            switch (status) {
                case EStatus.Sending:
                    context.Send(o => {
                        m_date_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Italic);
                        m_nick_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Italic);
                        m_message_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Italic);
                    }, null);
                    break;
                case EStatus.Sent:
                    context.Send(o => {
                        m_date_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Normal);
                        m_nick_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Italic);
                        m_message_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Italic);
                    }, null);
                    break;
                case EStatus.FailedToSend:
                    context.Send(o => { m_date_range.ApplyPropertyValue(TextElement.ForegroundProperty, Brushes.Red); }, null);
                    break;
                case EStatus.Delivered:
                    context.Send(o => {
                        m_date_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Normal);
                        m_nick_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Normal);
                        m_message_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Italic);
                    }, null);
                    break;
                case EStatus.Seen:
                    context.Send(o => {
                        m_date_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Normal);
                        m_nick_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Normal);
                        m_message_range.ApplyPropertyValue(TextElement.FontStyleProperty, FontStyles.Normal);
                    }, null);
                    break;
            }
        }
    }
}
